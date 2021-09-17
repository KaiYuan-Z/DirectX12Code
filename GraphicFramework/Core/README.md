# Core说明
Core是渲染框架的核心部分，负责：维护渲染管线及资源、管理场景数据、实现简单的GUI（基于IMGUI）、简单的APP框架支持。

<br>

<br>

## 1. 层次结构 
Core的代码主要包含GraphicsCore、SceneAndObjects、Utility和ShaderUtility四部分，其树状图如下：

``` 
  Core
    ├─GraphicsCore
    │   ├─GPUResource
    │   │   ├─Descriptors
    │   │   └─Resources
    │   ├─Pipeline
    |   |   ├─Signatures
    │   │   ├─PipelineState
    │   │   └─CommandManager
    │   ├─GUI
    │   │   ├─ImGui
    │   │   └─GuiManager
    │   └─GraphicsCore
    │       ├─Common
    │       ├─DXSample
    │       └─GraphicsCore
    ├─SceneAndObjects
    |   ├─Objects
    │   |   ├─Boundings
    │   |   ├─Cameras
    │   |   ├─Grass
    │   |   ├─Lights
    │   |   └─Models
    │   ├─Scene
    │   └─Render
    ├─Utility
    └─ShaderUtility
        ├─SceneRender
        └─Others
```

<br>

<br>

### 1.1 GraphicsCore
GraphicsCore主要负责：管理渲染资源（GPUResource部分)、维护渲染管线(Pipeline部分）、实现简单的GUI（GUI部分）、封装渲染管线相关接口并提供APP框架支持
（GraphicsCore部分），下面将对这4部分，分别进行说明。

<br>

<br>

#### 1.1.1 GPUResource
(1) Descriptors：<br>
负责管理DX12渲染资源的各种描述符（注：在DX12中资源描述符需要用户自行管理）。为适应各种使用情景，当前框架的资源描述符管理被分成了DescriptorAllocator(NonShaderVisible)、DynamicDescriptorHeap(ShaderVisible)和UserDescriptorHeap这3部分，其中：<br>

* DescriptorAllocator(NonShaderVisible)：<br>
无数量限制的，Shader不可见的，资源描述符分配器。当创建一个Gpu Resource后，为了使用该Gpu Resource还要为其创建各种View（如：Shader Resource View、Render Target View等），而创建各种View所需的描述符都从DescriptorAllocator中分配。此外需要说明的是，DescriptorAllocator使用的是Shader不可见的描述符堆，从中分配的描述符是Shader不可见的，它主要用于暂存各种资源的描述符，渲染时需要将这些描述符拷贝到Shader可见的描述符堆后，才可被Shader访问（注：Shader可见的描述符堆大小有限制，不可能将所有的资源描述符都存到Shader可见的堆上。而Shader不可见的描述符堆大小没有限制，所以平时都会将暂时不用的描述符存到Shader不可见的堆上）。

* DynamicDescriptorHeap(ShaderVisible)：<br>
Shader可见的，线性分配的，描述分配器。执行渲染时，若用户自己维护Shader可见的描述符堆，需要保证在Command List执行完之前，描述符堆中的描述符不能被修改，这往往需要创建多缓冲并使用Fence来同步，处理起来比较复杂，而将Command List配合Dynamic Descriptor Heap一起使用便可解决这个问题（Graphics Core封装的Command List内置了Dynamic Descriptor Heap，下文所提到的Command List都是指Graphics Core封装的Command List）：（a）Dynamic Descriptor Heap中维护了多个Shader可见的描述符堆，若Command List使用了Set Dynamic Descriptor，那么在执行绘制前Command List会将传入的描述符拷贝内置的Dynamic Descriptor Heap会将拷贝到当前描述符堆的下一个空闲位置，而不会覆盖已被使用的描述符，；（b）若当前的描述符堆空间不足时，Dynamic Descriptor Heap会自动申请一个新的描述符堆（可能来自回收的，也可能是新new出来的），然后在Command Queue中插入Fence，并将旧的描述符堆和Fence一起放入等待队列，只有当Fence被执行到以后才会回收旧的描述符堆；（c）当Command List调用了DrawCall或DispatchCall时，Command List会检测是否使用了Dynamic Descriptor Heap，是则自动将当前的Dynamic Descriptor Heap绑定到管线用于渲染。

* UserDescriptorHeap：<br>
用户自行维护的描述符堆，可Shader可见，也可Shader不可见。其中：UserDescriptorHeap是没有多缓冲没有Fence同步的简单描述符堆，比较适合设置一次后就永远不会修改的情况；SwappedUserDescriptorHeap是带多缓冲和Fence同步的描述符堆，适合应用在需要动态更新的情况。需要注意的是，SwappedUserDescriptorHeap是固定大小，且每次更新都必须Swap到下一个描述符堆，使用时需注意描述符堆的大小要尽量和每次要提交的描述符数量相匹配，否则会造成空间浪费。平常使用的话，还是Command List内置的Dynamic Descriptor Heap比较方便，只有在个别特殊的情况下才需要使用UserDescriptorHeap或SwappedUserDescriptorHeap（比如：若渲染要传递的描述符很多，且每帧都不会改变，那么这时使用UserDescriptorHeap会比较合适。因为可以免去，因使用Command List内置的Dynamic Descriptor Heap，而带来的每DrawCall一次的描述符拷贝）。

<br>

(2) Resources：<br>
主要包含的是，一些Gpu资源的定义，以及一些Gpu资源的分配器，其中：<br>

* GPUBuffer：<br>
各种GPU Buffer的定义，包括ByteAddressBuffer、ConstantBuffer、ReadbackBuffer、StructuredBuffer、UploadBuffer等。

* PixelBuffer：<br>
各种用于作RT的Buffer，包括：DepthBuffer、ColorBuffer等。

* Texture：<br>
贴图资源。

* LinearAllocator：<br>
DX12显存的线性分配器。LinearAllocator底层维护了一个显存的页分配池，LinearAllocator分配小块显存时是从固定大小的页上线性地分配（即先从显存页分配池分配一页的显存空间，然后再从这一页显存中线性地分配小块显存），LinearAllocator分配大块显存时是直接分配一个对应大小的大的显存页。LinearAllocator释放显存的方式比较直接，即直接释放其占有的所有显存页（其中，固定大小的显存页会归还给页分配池再利用，而大页显存会被直接释放）。LinearAllocator主要用于支持Command List（指的是Graphics Core封装的Command List）的SetDynamicConstantBufferView(...)、SetDynamicSrv(...)、SetDynamicVB(...)、SetDynamicIB(...)等函数，Command List内部维护有LinearAllocator，每次调用这些函数都会从LinearAllocator中线性分配显存并将分配的显存首地址赋给管线用于传递数据，当执行FinishCommandList时Command List会清除其占用的所有显存页（注：并不会立即释放这些显存页，而是放入显存页分配池的等待释放队列，待这部分页不再被Gpu占用后再执行回收利用或直接释放），通过这些函数用户无需自己维护Gpu Buffer便可将数据提交到渲染管线，而且使用这些函数时用户无需考虑Buffer在Cpu与Gpu之间的同步问题。

<br>

<br>

#### 1.1.2 Pipeline
(1) Signatures：<br>
RootSignature、CommandSignature <br>

<br>

(2) PipelineState：<br>
GraphicsPSO、ComputePSO <br>

<br>

(3) CommandManager：<br>
* CommandAllocatorPool：<br>
CommandAllocator的池，负责管理CommandAllocator的分配、复用、释放，为了方便管理目前本框架是一个CommandQueue对象配一个CommandAllocatorPool。在DX12中CommandAllocator负责储存CommandList记录的指令，一个CommandList对应一个CommandAllocator。调用ID3D12CommandQueue::ExecuteCommandLists后，CommandQueue会从CommandAllocator中读取指令。在Gpu执行完CommandList记录的指令之前，CommandAllocator中的数据必须保证不会被修改。CommandAllocator不能在多个线程间共享，需要一个CommandAllocator对应一个线程。

* CommandListManager：<br>
CommandList的管理器，负责管理CommandList的分配、复用、释放，内置4个CommandListPool（DIRECT、BUNDLE、COMPUTE、COPY每种类型的CommandList各对应一个Pool），全局只存在一个CommandListManager对象（储存在CommandManager中）。在DX12中，渲染管线的指令都通过CommandList来记录。当向CommandList记录指令时，指令并不会被立即提交，只有调用ID3D12CommandQueue::ExecuteCommandLists后指令才会被提交给Gpu。CommandList不能在多个线程间共享，需要一个CommandList对应一个线程。

* CommandQueueManager：<br>
CommandQueue的管理器，其内部维护了3个CommandQueue（DIRECT、COMPUTE、COPY每种类型各一个），全局只存在一个CommandQueueManager对象（储存在CommandManager中）。在DX12中，CommandQueue负责执行CommandList，同时也负责CPU与GPU同步（同步的基本原理是:向CommandQueue中插入Fence，若CommandQueue已执行到了Fence，就说明Fence之前的Command都已执行完毕，再将Fence机制与操作系统的信号量等机制相结合便可实现CPU与GPU的同步）。CommandQueue可以在多个线程间共享。

* CommandManager：<br>
将CommandAllocatorPool、CommandListManager、CommandQueueManager整合在一起管理，并提供一些简洁易用的接口供外部调用。

<br>

(4) 一些补充说明：<br>
* CommandManager::BeginCommandList(...)：<br>
当CommandListManager中没有空闲CommandList时，会new一个新的CommandList，其他时候只是从空闲队列里弹出一个空闲的CommandList。此外，调用BeginCommandList时会导致CommandList被重置。

* CommandList::SetDynamicDescriptor(...)：<br>
（a）使用此方法，便可以利用CommandList对象内置的DynamicDescriptorHeap来传递Descriptor到渲染管线，这样可以免去用户自己去创建和维护DescriptorHeap（用户自己维护DescriptorHeap需要保证在CommandList执行完之前DescriptorHeap中的Descriptor不被修改，维护起来比较繁琐）。<br>
（b）每个DrawCall最大可以设置256个Descriptor，当调用DrawCall时Descriptor会被拷贝到CommandList对象内置的DynamicDescriptorHeap上。CommandList对象内置的DynamicDescriptorHeap大小为1024，DynamicDescriptorHeap是被连续使用的，每次新拷贝的Descriptor会放在DynamicDescriptorHeap的下一个空闲位置。当前的DynamicDescriptorHeap被撑满时，CommandList会自动申请一个新的（可能来自回收的，也可能是新new出来的），并将旧的DynamicDescriptorHeap放入等待队列，等到其不再被Gpu所占用，才会被回收再利用。<br>

* CommandList::SetDynamicConstantBufferView(...)/SetDynamicSrv(...)/SetDynamicVB(...)/SetDynamicIB(...)：<br>
（a）使用此类方法，便可以利用CommandList对象内置的LinearAllocator来传递数据到管线，这样可以避免用户自己创建和维护UploadBuffer（用户自己维护UploadBuffer的话，需要保证在CommandList执行完成前UploadBuffer不被修改，维护起来比较繁琐）。<br>
（b）LinearAllocator的显存分配策略：当用户数据比较小时（一般小于2MB），从固定大小的页上线性地分配（即先从显存页分配池分配一页的显存空间，然后再从这一页显存中线性地分配小块显存）。当用户数据比较大时每次都直接重新分配新的显存，使用完后不回收而是直接释放。<br>
（c）使用这类方法传递数据,每次都会触发数据拷贝，此外若单次传递的数据大于一页的大小时还会触发显存分配。当Buffer的更新频率比较低或Buffer比较大时使用此类方法传递数据效率较低，此种情况下还是用户自行控制Buffer的分配和更新比合适。<br>

* CommandList::SetDynamicViewDescriptorTemporaryUserHeap(...)：<br>
（a）临时设置自定义的Dynamic描述符堆，下一次触发绑定Dynamic描述符堆时会使用本函数指定的堆（注：只影响一次，所以叫临时设置）。<br>
（b）调用CommandList的DrawCall/DispatchCall/CommitCachedDispatchResourcesManually时，会触发绑定Dynamic描述符堆。<br>
（c）该函数要配合SetDynamicDescriptor使用。不使用SetDynamicDescriptor时，请直接使用SetDescriptorHeap设置描述符堆。<br>

* CommandList::DrawCall(...)/DispatchCall(...)：<br>
（a）清空GPU资源屏障，拷贝用户设置的Descriptor到DynamicDescriptorHeap（如果之前有调用SetDynamicDescriptor的话），执行DrawCall或DispatchCall。<br>
（b）DX12绘制时只能设置一个描述符堆，若提交绘制之前使用过SetDynamicDescriptor，那么CommandList对象会自动将DynamicDescriptorHeap或UserDynamicDescriptorHeap设置到管线。一旦使用了SetDynamicDescriptor，CommandList对象将自动忽略用户通过SetDescriptorHeap设置的描述符堆，因此SetDynamicDescriptor和SetDescriptorHeap。<br>

* CommandManager::FinishCommandList(...)：<br>
当FinishCommandList被调用后，CommandList会Clear所有使用过的DynamicDescriptorHeap和LinearAllocationPage，这样可以保证每次BeginCommandList时CommandList使用的都是空的DynamicDescriptorHeap和LinearAllocationPage（注：回收并不会立即清空它们，而只是将它们压入回收队列，待GPU Command真的执行完毕以后才会将它们清空。若BeginCommandList时所有的DynamicDescriptorHeap和LinearAllocationPage都还在被Gpu占用，则会分配新的）。

* CommandList使用注意事项：<br>
（a）CommandList尽量多录入，并减少提交次数，避免过多的重置和提交CommandList的开销。<br>
（b）录入CommandList指令时，需要注意保证DescriptorHeap和各种GpuBuffer在指令执行完毕前不能被修改。对于需要动态修改的数据推荐使用SetDynamicXxxx这类函数，它们能有效避免多个DrawCall间数据被覆盖的问题。<br>
（c）注意SetDynamicDescriptor功能会覆盖用户设置的描述符堆，使用时需注意。<br>
（d）使用SetDynamicDescriptor提交描述符后，在DrawCall或DispatchCall时会触发Cache到内置DynamicDescriptorHeap的拷贝，一般只传递几个到十几个描述符的情况直接使用SetDynamicDescriptor没有什么问题。而若光追场景渲染那种要传递几十甚至上百张贴图的描述符的情况，不如提前将贴图的描述符拷贝到一个用户自定义的UserDescriptorHeap上，每次直接Set Descriptor Heap，这样可以避免拷贝。<br>
（e）若使用了UserDescriptorHeap的情况下，还想使用SetDynamicDescriptor功能，可以使用SetDynamicViewDescriptorTemporaryUserHeap + SetDynamicDescriptor的组合：固定不变的描述符储存在UserDescriptorHeap前部，在SetDynamicViewDescriptorTemporaryUserHeap时利用StartOffset跳过它们，这样SetDynamicDescriptor设置的Descriptor会在DrawCall或DispatchCall时拷贝到固定不变的描述符的后面，进而可实现将UserDescriptorHeap和SetDynamicDescriptor组合使用（注：为保证GPU正在使用的Heap不会被覆盖，UserDescriptorHeap需要做相关同步处理）。<br>
（f）ResourceBarrier的使用策略：每个Resource在使用前设置状态转换，而使用后不负责后续的转换，这样可以避免重复设置。<br>

<br>

<br>

#### 1.1.3 GUI
基于ImGui实现的简易UI框架，即为ImGui封装了一个简单的UIManager，以便将ImGui接入到GraphicsCore中。全局只有一个UIManager，由GraphicsCore维护：<br>
* GraphicsCore在Initialize时会调用UIManager::InitGui来初始化ImGui。<br>
* 用户通过UIManager::RegisterGuiCallbackFunction可以注册自己的UI配置函数，用于更新自己的GUI布局。<br>
* 调用GraphicsCore::FinishCommandListAndPresent后，GraphicsCore会在执行D3D12的Present前处理Gui的绘制，绘制Gui的流程是：先调用用户注册的GuiCallbackFunction来更新Gui布局，然后调用GuiManager::RenderGui来提交Gui的绘制指令。<br>
* GraphicsCore在Shutdown的时候会调用UIManager::DestroyGui来销毁ImGui的资源。<br>

<br>

<br>

#### 1.1.4 GraphicsCore
* 框架暴露给用户的API接口（GraphicsCore）
* 留给用户继承的App框架（DXSample）
* 使用框架所需包含的头文件（Common）

<br>

<br>

### 1.2 SceneAndObjects
场景管理和绘制部分，主要分为3类：Objects（场景中对象）、Scene（逻辑场景）、Render（场景的各种Render）

<br>

<br>

#### 1.2.1 Objects
渲染场景所用到的各种对象，这些对象都由Scene来管理，可通过c++接口或配置文件来配置。其中：
* Boundings：各种包围体的定义 <br>
* Cameras：各种Camera的定义，每种Camera的定义都必须继承自CameraBasic <br>
* Grass：实时计算的草地，参考自D3D12 Raytracing Real-Time Denoised Ambient Occlusion中的草地，当初做Demo时间比较紧，因此只搬过来了LOD 0 <br>
* Lights：各种光源的定义 <br>
* Models：模型的定义（其中：SDFModel的支持是实验室老哥贡献的）、模型Instance的支持、模型加载的支持 <br>

<br>

#### 1.2.2 Scene
Scene负责管理逻辑场景，ModelInstances、Lights、Grass和Cameras等Objects都由Scene来统一管理。Scene支持通过Json格式的配置文件进行配置，而且在RasterizerSamples的ModelViewer(SimpleSceneEditor)中还实现了一个简易的场景编辑器，支持配置场景中的Model Instances和Grass。<br>

<br>

#### 1.2.3 Render
SceneRender用于提供场景绘制支持，它需要用Scene来初始化。SceneRender实际主要负责：管理场景的渲染数据、实现场景的绘制逻辑（传参和调用DrawCall）、提供XxxRenderUtil.hlsl（一个Shader帮助库，用于提供SceneRender相关的Uniform参数定义、光照函数定义、VSInput结构定、一些工具函数定义）。在实现Demo或Effect时使用SceneRender，可以快速构建场景的渲染逻辑和渲染Shader，进而提高开发效率。目前已提供的SceneRender有：<br>
* SceneForwardRender：Forward的场景渲染器，支持BlinnPhong和PBR两种Shading，对应的RenderUtil为SceneForwardRenderUtil.hlsl <br>
* SceneGBufferRender：场景GBuffer的渲染器，支持BlinnPhong和PBR两种GBuffer，对应的RenderUtil为SceneGBufferRenderUtil.hlsl <br>
* SceneGeometryOnlyRender：只输出场景几何信息的渲染器，用于实现Shadow或者一些只需要场景几何GBuffer的算法Demo(如：AO)，对应的RenderUtil为SceneGeometryOnlyRenderUtil.hlsl <br>
* SceneLightDeferredRender：用于Deferred管线渲染光照，支持BlinnPhong和PBR两种Shading，对应的RenderUtil为SceneLightDeferredRenderUtil.hlsl <br>
* SDFSceneRender：场景SDF的渲染器，其代码实现由实验室老哥贡献，对应的RenderUtil为SDFSceneRenderUtil.hlsl <br>

<br>

<br>

### 1.3 Utility和ShaderUtility
* Utility：各种工具（数学、Json、文件读写、Hash、Object容器等）
* ShaderUtility：除了SceneRender的ShaderUtil之外，其他也是各种工具函数（球面采样、半球采样、球面编码、Halton、建立基底、VisibilityBuffer等）

<br>

<br>

## 2. 使用说明
### 2.1 整体流程：
（1）通过继承CDXSample，实现Demo框架 <br>
（2）在main函数里使用GraphicsCore运行Demo <br>

<br>

### 2.2 示例代码：
#### 2.2.1 Demo.h/Demo.cpp
```cpp
//=================================================
// 实现Demo框架
//=================================================
class CDemo : CDXSample 
{
public:
  CDemo(std::wstring name) : CDXSample(name)
  {
  }
  
  ~CDemo()
  {
  }

  //
  // 实现初始阶段的虚函数
  //
  virtual void OnInit() override
  {
      //
      // 注册GUI回调函数，用于更新GUI
      //
      GraphicsCore::RegisterGuiCallbakFunction(std::bind(&CDemo::__UpdateGUI, this));
      
      //
      // 从配置文件加载并初始化Scene
      //
      m_Scene.LoadScene(L"scene.txt");
      
      //
      // 初始化SceneRender
      //
      SSceneForwardRenderConfig RenderConfig;
      RenderConfig.pScene = &m_Scene;
      ......
      m_SceneRender.InitRender(RenderConfig);
      
      //
      // 初始化RootSignature（包括：m_SceneRender参数的Signature，以及DemoShader自己要传递参数的Signature）
      //
      m_RootSignature.Reset(m_SceneRender.GetRootParameterCount() + 1, m_SceneRender.GetStaticSamplerCount());
      m_SceneRender.ConfigurateRootSignature(m_RootSignature);
      UINT NextSignatureIndex = m_SceneRender.GetRootParameterCount();
      m_RootSignature[NextSignatureIndex].InitAsConstantBuffer(...);
      m_RootSignature.Finalize(...);
      
      //
      // 初始化PSO
      //
      m_PSO.SetXxx(...);
      ......
      m_PSO.Finalize();
      
      //
      // CPU等待渲染资源初始化完成
      //
      GraphicsCore::IdleGPU();
  }
  
  //
  // 实现键鼠响应相关的虚函数
  //
  virtual void OnKeyDown(UINT8 key) override { ....... }
  virtual void OnMouseMove(UINT x, UINT y) override { ....... }
  virtual void OnRightButtonDown(UINT x, UINT y) override { ....... }
  
  //
  // 实现Update虚函数（会每帧调用，且在OnRender之前被调用）
  //
  virtual void OnUpdate() override
  {
      m_Scene.OnFrameUpdate();
  }
  
  //
  // 实现Render虚函数（会每帧调用）
  //
  virtual void OnRender() override
  {
      // 获取CommandList
      CGraphicsCommandList* pGraphicsCommandList = GraphicsCore::BeginGraphicsCommandList();

      // 设置GPU资源屏障
      pGraphicsCommandList->TransitionResource(...);
      ......
      
      // 重置RT
      pGraphicsCommandList->ClearColor(...);
      pGraphicsCommandList->ClearDepth(...;
      
      // 设置各种渲染状态和渲染参数
      pGraphicsCommandList->SetXxx(...);

      // 调用SceneRender的绘制函数
      m_SceneRender.DrawScene(pGraphicsCommandList);
      
      // 其他可能的DrawCall
      pGraphicsCommandList->DrawXxx(...);
      
      // 执行CommandList并Present到RT（该函数内部会先触发GUI绘制，然后才会提交执行CommandList并Present到RT）
      GraphicsCore::FinishCommandListAndPresent(pGraphicsCommandList);
  }
  
  //
  // 实现Sample销毁时的回调（程序关闭时会执行各种Destroy流程，其Destroy的整体顺序是：先调用CDXSample的OnDestroy，然后再释放CDXSample对象，最后释放各种资源）
  //
  virtual void OnDestroy() override
  {
      /*
          GraphicsCore的各种资源的析构函数都自带释放各种资源的代码。如果CDXSample使用的资源以CDXSample成员变量的形式被定义，而且这些资源不需要按照特定顺序释放，         
          那么CDXSample::OnDestroy可以不用实现，因为CDXSample对象的释放是在GraphicsCore其他资源释放之前，靠其析构函数释放资源并不会出现问题。虽然这样用不太好，
          但毕竟写起来比较省事。     
      */
  }

private:
   //
   // 实现GUI更新回调函数
   //
   void __UpdateGUI()
   {
      ......
   }

private:
   // 场景对象
   CScene m_Scene;
  
   // 场景渲染器
   CSceneForwardRender m_SceneRender;
  
   // PSO对象
   CGraphicsPSO m_PSO;
  
   // 根签名对象
   CRootSignature m_RootSignature;
}
```

<br>

#### 2.2.2 Main.cpp
```cpp
//=================================================
// 运行Demo
//=================================================
#include "Demo.h"

int main(int argc, wchar_t** argv)
{
    //
    // 初始化GraphicsCore
    //
    GraphicsCore::SGraphicsCoreInitDesc InitDesc;
    InitDesc.WinConfig = GraphicsCore::SWindowConfig(0, 0, 1600, 900);
    InitDesc.pDXSampleClass = new CDemo(L"Demo"); // 创建DXSample对象并传递给GraphicsCoreInitDesc（用户不用关心DXSample对象的销毁问题，GraphicsCore会自行销毁它）
    InitDesc.ShowFps = true;
    InitDesc.EnableGUI = true;
    GraphicsCore::InitCore(InitDesc);
  
    //
    // 运行GraphicsCore
    //
    return GraphicsCore::RunCore();
}
```
