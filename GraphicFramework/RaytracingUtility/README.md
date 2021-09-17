# RaytracingUtility说明
* RaytracingUtility是为Core提供光线追踪支持的扩展库，若Demo程序需要使用光线追踪特性，则需要同时引用Core和RaytracingUtility两个库。
* 将光追代码从Core中提出来主要是为了：（1）保持Core的简洁性，避免将用不上的光追代码，带入到非光追的程序中。（2）避免因引入光追功能，而导致原有渲染管线的接口和使用方式发生改变。
* 由于DXR的光追管线和传统的DX12渲染管线差别有点大，因此在介绍RaytracingUtility实现框架之前，需要先对DXR的光追管线进行简要介绍，以便能更好的理解RaytracingUtility各个部分的作用。

<br>

<br>

## 1. 简述DXR的光追管线
DXR作为DirectX 12的扩展，它的光追管线可以与DirectX 12的其他管线共享Texture、Buffer、Constant等GPU资源，并能够通过DirectX 12的Command List派发光线追踪任务。DXR的光追管线新引入了3种结构：<br>
* Ray Tracing Pipeline State Object：光追管线的PSO，包含了编译好的光追Shader代码，以及一些光追管线的配置参数。<br>
* Acceleration Structures：光追场景的加速结构，用于加速光线与场景求交，光追时它作为一种特殊的Shader Resource传递给Shader。<br>
* Shader Table：用来储存光追时需要调用的Shader，其内部可包含多条Shader Record，每条Shader Record都包含一个Shader ID，以及Shader对应的根参数。<br>

<br>

<br>

### 1.1 Acceleration Structures
为了加速光线与场景的求交，DXR使用了基于BVH树的加速结构来管理整个场景。该加速结构主要包括：Bottom Level Acceleration Structure（简称：BLA）和Top Level Acceleration Structure（简称：TLA）两层，其中：<br>
* BLA：BLA由诸如三角形之类几何图元构建而来。而输入到BLA的图元数据由一个或多个Geometry Descriptor来指定，其中每个Geometry Descriptor都对应一个Vertex Buffer和一个Index Buffer。<br>
* TLA：TLA由一个或多个Instance构建而来，构建时每个Instance通过Instance Descriptor来描述。每个Instance Descriptor包含有，Instance的Transformation矩阵、Instance引用的BLA的显存地址、Instance对应的Shader Table Offset。<br>

TLA、BLA、Shader Table三者的对应关系如下图所示：<br>
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/AccelerationStructuresAndShaderTable.png" width="750"/></div>

<br>

<br>

### 1.2 Ray Tracing Pipeline
DXR的光追管线总共包含5个Shader：Ray Generation Shader、Intersection Shader、Any Hit Shader、Closest Hit Shader、Miss Shader。其中：<br>
* Ray Generation Shader：该Shader用于向场景派发光线（通过在Shader内调用TraceRay()来实现）。<br>
* Intersection Shader：该Shader定义了光线与任意类型图元求交的计算方法。<br>
* Any Hit Shader：该Shader主要用于Alpha Test，它能够根据需要丢弃或接受由Intersection Shader得出的求交结果。<br>
* Miss Shader Shader：当投射的光线搜索完整个场景仍未找到与其相交的物体时，该Shader便会被执行。<br>
* Closest Hit Shader：当投射的光线搜索到距离其起点最近的交点时，该Shader便会被执行。<br>

光追时每个GPU线程负责一根光线，线程之间不能相互通信，每条光线的光追流程都是由这5个Shader来实现。一根光线的光追流程如下图所示：<br>
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/RtPipeline.png" width="400"/></div>

<br>

此外，需要说明的是，在光追管线中，既有所有Shader都能访问的Global参数，也有只有对应Shader才能访问Local参数，这两种类型的参数分别对应了Global Root Signature和Local Root Signature。Global参数通常用来指定全局使用的参数，如：场景加速结构的SRV、用于存储光追结果的UAV等。而Local参数通常用来指定某个Shader自己使用的参数，如：在Closest Hit Shader中常常需要指定对应 Geometry的MeshID、ModelID、以及相关贴图资源，这时便需要用Local参数来指定（注：在光追场景中，每个Geometry都有与自己相对应Shader，具体说明可参看Shader Table的有关内容）。

<br>

<br>

### 1.3 Shader Table
Shader Table是将场景中Object与Shader联系在一起的数据结构，它定义了哪个Shader要与场景中的哪个Object相关联。Shader Table由一组大小相等的Shader Record构成，每条Shader Record都包含一个Shader ID（对应一个Shader或一个Hit Group），以及Shader对应的一系列根参数（即Root Arguments，这些Root Arguments由Shader自己的Local Root Signature来描述）。Shader Table与Shader Record的示意图如下：<br>
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/ShaderTable.png" width="750"/></div>

<br>

在Shader Record中，一个RootBufferView（即：CBV、SRV、UAV）记录的是对应Buffer的D3D12_GPU_VIRTUAL_ADDRESS（需要8字节对齐），一个RootDescriptorTable记录的是对应DescriptorTable的第一个Descriptor的D3D12_GPU_DESCRIPTOR_HANDLE（需要8字节对齐），一个RootConstant直接记录的是要传递的4字节数值（需要4字节对齐）。透过Shader Record的内部实现，大概可以猜出DX12的CommandList设置BufferView、DescriptorTable、Constant这些参数时实际传递的是什么，也能看出：Shader读取Constant时是直接读取数值（需要0次跳转）、Shader读取BufferView需要一次地址跳转、Shader读取DescriptorTable中某个Descriptor时需要两次跳转（先跳转到DescriptorTable首地址，再根据偏移跳转到对应的Descriptor）。<br>

<br>

在调用DispatchRays时，Shader Table会通过D3D12_DISPATCH_RAYS_DESC传入到光追管线，其中：Ray Generation Shader只能指定一个Shader Record（即一次DispatchRays只能使用一个Ray Generation Shader），Hit Group Shader可以指定一个Shader Table，Miss Shader可以指定一个Shader Table。传给D3D12_DISPATCH_RAYS_DESC的Shader Table描述仅是StartAddress + SizeInBytes + StrideInBytes的组合，因此Ray Generation、Hit Group、Miss三者的Shader Table既可以各自存入单独的GPU Buffer中，也可以都放在同一个GPU Buffer中。

<br>

在光追管线中，对于由TraceRay(...)发射一根光线，它在Ray Generation、Hit、Miss的各个阶段具体要调用哪个Shader，有着各自不同的固定计算方式：<br>
* Ray Generation：DispatchRays时只能指定一个Shader Record，不用计算。

* Miss：___&M[0] + ( sizeof(M[0]) * I<sub>miss</sub> )___，其中 <br>
" M "代表Miss Shader Talbe,  <br>
" &M[0] "为Miss Shader Talbe的首地址， <br>
" sizeof(M[0]) "为Miss Shader Talbe的一条Shader Record的大小， <br>
" I<sub>miss</sub> "为TraceRay(...)的MissShaderIndex参数。 <br>

* Hit Group：___&H[0] + ( sizeof(H[0]) * (I<sub>ray</sub> + G<sub>mult</sub>*G<sub>id</sub> + I<sub>offset</sub>) )___，其中 <br>
" H "代表Hit Group Shader Talbe,  <br>
" &H[0] "为Hit Group Shader Talbe的首地址， <br>
" sizeof(M[0]) "为Hit Group Shader Talbe的一条Shader Record的大小， <br>
" I<sub>ray</sub> "和" G<sub>mult</sub> "分别为TraceRay(...)的RayContributionToHitGroupIndex和MultiplierForGeometryContributionToHitGroupIndex参数， <br>
" G<sub>id</sub> "为Geometry的ID（创建Instance时，系统会为Instance关联的每个Geometry自动分配ID，一般是从0开始递增的分配）， <br>
" I<sub>offset</sub> "创建Instance时通过Instance Descriptor传入的InstanceContributionToHitGroupIndex（是Per-Instance的Offset）。 <br>

在实际应用中，" G<sub>mult</sub> "常代总共拥有多少种Hit Prog（Hit Group Shader Program），" I<sub>ray</sub> "常代表要使用第几种Hit Prog，而一个Instance所占有的Shader Record总数为：InstanceGeometryCount * G<sub>mult</sub>，也就是说每个Instance的每个Geometry都要有与其对应的Shader Record（注：这样做主要是为了Hit Shader能够方便获取对应Hit Object的相关属性）。那么对于一个复杂场景来说，它的Hit Group Shader Table往往需要有成百上千条Shader Record。基于上述计算方法，为场景中所有的Instance构建Hit Group Shader Table的基本流程为：
```cpp
UINT TalbeRecordIndex = 0;
for each [SceneInstances]
{
    for each [InstanceGeometrys]
    {
        for each [HitProgTypes]
        {
            HitGroupShaderTable[TalbeRecordIndex++] = SetRecordData(...);
        }
    }        
}
    
```

<br>

<br>

### 1.4 Ray Tracing整体流程
* （1）初始化Direct12 Device，并检测其是否支持光追。
* （2）为渲染场景建立Acceleration Structures。
* （3）加载并编译Shader代码。
* （4）定义Root Signatures和Ray Tracing Pipeline State Object。
* （5）创建Shader Table。
* （6）传递光追相关参数，并Dispatch光追任务。

<br>

从上面的流程可以看出，光追管线的运作流程与计算管线十分类似。除了Shader Table可能在传统的计算管线中找不到对应关系之外，其他部分的使用方式都和计算管线非常类似，其中：Acceleration Structures可以看作绑定到计算管线的一种特殊Shader Resource、而Ray Tracing PSO可以与计算管线的PSO相对应。这说明DXR的管线设计得很巧妙，在引入了光追所需的各种资源的同时，又能做到让光追管线的使用流程和以前的计算管线尽量保持一致，有利于降低用户的学习成本和接入成本。

<br>

<br>

## 2. RaytracingUtility说明
### 2.1 需求分析
虽然DXR的光追管线是一条全新的管线，但是光追管线可以与其他管线共享Texture、Buffer、Constant等GPU资源，而且光追管线的Device和CommandList类型都是从非光追的DX12类型继承而来，除了新加入的光追相关接口之外，使用的还是非光追的Device和CommandList的接口。所以对于光追Demo来说，它的大部分渲染框架仍可依赖Core来构建，只有涉及光追相关的资源和接口时才需要RaytracingUtility提供支持。需要RaytracingUtility提供的支持有：<br>
* （1）进一步封装Scene和SceneRender，让场景的管理和渲染支持光线追踪。<br>
* （2）对DXR原生的Acceleration Structures、Ray Tracing Pipeline State Object、Shader Table等对象进行封装，以方便上层使用。<br>
* （3）提供工具函数，以便从Core创建的Device和CommandList中Query出光追的Interface。<br>

<br>

<br>

### 2.2 框架说明
RaytracingUtility的代码主要包含RaytracingAccelerationStructure、RaytracingScene、RaytracingPipeline和ShaderUtility四部分，其树状图如下：

``` 
  RaytracingUtility
    ├─RaytracingAccelerationStructure
    │   ├─BottomLevelAccelerationStructure
    |   ├─RaytracingInstanceDesc
    │   └─TopLevelAccelerationStructure
    ├─RaytracingScene
    |   ├─RaytracingScene
    │   └─RaytracingSceneRenderHelper
    ├─RaytracingPipeline
    │   ├─RaytracingShaderTable
    |   ├─RaytracingPipelineStateObject
    |   ├─RaytracingDispatchRaysDescriptor
    │   └─RaytracingPipelineUtility
    └─ShaderUtility
        ├─RaytracingSceneRenderHelper
        └─RaytracingUtil
```

<br>

<br>

#### 2.2.1 RaytracingAccelerationStructure
* BottomLevelAccelerationStructure：<br>
对BLA进行封装，以便提供从VertexBuffer和IndexBuffer中创建BLA、从Model数据中创建BLA、更新可形变Mesh的BLA等支持。<br>

* RaytracingInstanceDesc 和 TopLevelAccelerationStructure：<br>
对InstanceDesc和TLA进行封装，以便提供为一组Instance创建TLA、更新Instance位置、更新TLA等支持。<br>

<br>

<br>

#### 2.2.2 RaytracingScene
* RaytracingScene: <br>
（1）由于光追光线需要使用加速结构来进行光线求交，因此RaytracingScene在初始化时，会自动为每个Model都建立一个BLA，并基于这些BLAs和所有Instances为场景建立TLA。
（2）在传统的光栅管线中，一般每次绘制只需将一个Model及其对应Instances的数据到管线。而到了光追管线，由于在光追时一根光线可以打到场景中的任何物体，因此相关的Shader在光追时必须能够获取到整个场景的任何数据。RaytracingScene做为Scene的光追扩展，它会收集场景里的Model数据、Instance数据、Light数据等，并将这些数据汇总成一个个总的GPU Buffer，以便供给光追Shader使用。<br>
（3）由**“ 1.3 Shader Table ”**的有关介绍可知，场景中的每个Instance都要有与其对应的Hit Group Shader Records（注：一个Instance要对应有 InstanceGeometryCount * G<sub>mult</sub> 条Shader Record）。所以RaytracingScene提供了自动为场景中所有Instance建立对应Hit Group Shader Records的功能，而Instance、Geometry、HitProgType与Hit Group Shader Record的具体映射关系可参看 **“ 1.3 Shader Table ”** 。<br>

* RaytracingSceneRenderHelper：<br>
（1）负责配置场景渲染所需的Root Parameter。<br>
（2）为了统一操作习惯，RaytracingScene的BuildHitShaderTable的功能，也由RaytracingSceneRenderHelper来负责暴露接口，以便供给外部调用。<br>
（3）负责初始化基于Halton序列的随机样本。<br>

<br>

<br>

#### 2.2.3 RaytracingPipeline
* RaytracingShaderTable：<br>
负责封装Shader Table相关资源，以方便上层使用，包括：初始化Shader Table的存储空间、提供设置Shader Record的各种参数的接口等（注：光追时传递给DXR管线的Shader Table描述仅是 StartAddress + SizeInBytes + StrideInBytes 的组合，DXR管线并没有提供具体的结构和接口来帮助上层来设置Shader Table，上层需要按照Shader Table的内存结构自己往Buffer里写数据，所以为了便于使用，非常有必要封装一下）。<br>
* RaytracingPipelineStateObject：<br>
负责封装Raytracing Pipeline State Object相关资源，以方便上层使用。DXR的Pipeline State Object需要使用STATE_OBJECT和STATE_SUB_OBJECT来配置各种参数，直接使用的话配置起来比较麻烦，封装一下有利于简化操作。<br>
* RaytracingDispatchRaysDescriptor：<br>
对D3D12_DISPATCH_RAYS_DESC进行封装，以便能配合RaytracingUtility::RaytracingShaderTable一起使用，让上层使用起来更方便。<br>
* RaytracingPipelineUtility：<br>
为上层提供工具函数，包括：帮助上层从Core的Device和CommandList中Query光追的Interface、帮助上层调用DispatchRays等。<br>

<br>

<br>

#### 2.2.4 ShaderUtility
光线追踪Shader的各种帮助库，用于帮助用户快速构建光追Shader代码。
* RaytracingSceneRenderHelper：场景渲染相关的各种资源定义和各种工具函数。
* RaytracingUtil：光追相关的其他工具函数。

<br>

<br>

## 3. 使用说明
在引用了RaytracingUtility库以后，除了新加入了一些光追资源的初始化流程以外，Core框架的使用流程没有太多变换，具体使用细节可参看：
* [Core](https://github.com/KaiYuan-Z/DirectX12Code/edit/master/GraphicFramework/Core) 的说明
* [RaytracingSamples](https://github.com/KaiYuan-Z/DirectX12Code/edit/master/RaytracingSamples) 的代码
