# GraphicFramework And Demos

## 1. [GraphicFramework](https://github.com/KaiYuan-Z/DirectX12Code/edit/master/GraphicFramework)
GraphicFramework是为了便于编写Demo代码，而实现的DX12框架，该框架主要负责：维护DX12渲染管线及相关资源，管理场景数据，实现基本的GUI，搭建简洁方便的APP框架，快速复用常见的渲染算法，提供DXR光追相关支持（框架实现参考了Microsoft DX12 Demo中的Mini Engine，以及Nvidia DX12 Demo中的Falcor）。

该框架主要包含以下3个静态库：<br>
* Core：<br>
渲染框架的核心部分，主要负责：维护渲染管线及资源、管理场景数据、实现简单的GUI（基于IMGUI）、简单的APP框架支持。

* RaytracingUtility：<br>
为Core提供光线追踪支持的扩展库，主要负责：增加光追相关的接口和资源、增加场景的光追支持、提供光追相关的Shader帮助库。

* Effects：<br>
常用图形算法库。在这个库中常用的图形算法都被封装成了一个个单独的Effect（包括：GBuffer、Shadow、AO、Blur等等），以方便在各个Demo程序间复用。

<br>

## 2. [RasterizerSamples](https://github.com/KaiYuan-Z/DirectX12Code/edit/master/RasterizerSamples)
基于光栅管线实现的程序，主要是一些光栅框架使用示例，以及一些基于光栅管线的Demo。<br>

<div align=left><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/D3D12_Framework.png" width="200"/> <img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/DeferredPbrModelView.png" width="200"/> <img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/ModelViewer(SimpleSceneEditor).png" width="200"/></div></div>

<br>

## 3. [RaytracingSamples](https://github.com/KaiYuan-Z/DirectX12Code/edit/master/RaytracingSamples)
基于光追管线实现的程序，主要是一些光追框架使用示例，以及一些基于光追管线的Demo（目前主要是一些光追AO的Demo）。<br>


<div align=left><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/RayTracingAO_RasterizerGBuffer_Halton_Cosine.png" width="200"/> <img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/TemporalRayTracingAO_CrossroadDemo2.png" width="200"/> <img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/TemporalRayTracingAO_CrossroadDemo1.png" width="200"/></div></div>

## 4. Resources
整个项目使用的模型和贴图等资源文件比较大，超过了github提供的lfs免费空间的限制，所以需要另外下载：https://pan.baidu.com/s/1Frg84tr9t-QHPqb2UhvYFw ，提取码：1234。下载完毕后直接将Resources.zip解压到DirectX12Code根目录即可。
