# GraphicFramework And Demos

## 1. [GraphicFramework](https://github.com/KaiYuan-Z/DirectX12Code/edit/master/GraphicFramework)
GraphicFramework是为了便于编写Demo代码，而实现的DX12框架，该框架主要负责：维护DX12渲染管线及相关资源，管理场景数据，实现基本的GUI，搭建简洁方便的APP框架，快速复用常见的渲染算法，提供DXR光追相关支持（框架实现参考了Microsoft DX12 Demo中的Mini Engine，以及Nvidia DX12 Demo中的Falcor）。

该框架主要包含以下3个静态库：<br>
* [Core](https://github.com/KaiYuan-Z/DirectX12Code/edit/master/GraphicFramework/Core) <br>
渲染框架的核心部分，主要负责：维护渲染管线及资源、管理场景数据、实现简单的GUI（基于IMGUI）、简单的APP框架支持。

* [RaytracingUtility](https://github.com/KaiYuan-Z/DirectX12Code/edit/master/GraphicFramework/RaytracingUtility) <br>
为Core提供光追支持的扩展库。将光追相关支持单独封成一个库主要考虑到：（1）把光追支持单独提出来，可以避免将Core搞复杂，也省得编译非光追Demo时还得带着用不上的光追代码一块编译。（2）此外这样做还能避免光追代码侵入到原有渲染框架，进而可保证Core原有的框架和接口不发生改变，降低光追接入成本。（3）DXR本就是DX12的扩展，光追的Device和CommandList都可通过QueryInterface的方式从已创建的Device和CommandList中获取（当然直接创建也行，只是对于扩展库来说，用QueryInterface更方便），这样Core只需提供非光追的Device和CommandList给RaytracingUtility即可，而Core不需要包含任何光追代码。

* [Effects](https://github.com/KaiYuan-Z/DirectX12Code/edit/master/GraphicFramework/Effects) <br>
常用图形算法库。在这个库中常用的图形算法都被封装成了一个个单独的Effect（包括：GBuffer、Shadow、AO、Blur等等），以方便在各个Demo程序间复用。此外，封装Effect库，除了方便复用之外，也是为了让Core足够精简、稳定和灵活，省得让Core整合进太多东西，导致其学习、维护以及Debug的成本过高。

<br>

## 2. [RasterizerSamples](https://github.com/KaiYuan-Z/DirectX12Code/edit/master/RasterizerSamples)
基于光栅管线实现的程序，主要是一些光栅框架使用示例，以及一些基于光栅管线的Demo。<br>

<div align=left><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/D3D12_Framework.png" width="200"/> <img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/DeferredPbrModelView.png" width="200"/> <img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/PbrModelViewer.png" width="200"/></div></div>

<br>

## 3. [RaytracingSamples](https://github.com/KaiYuan-Z/DirectX12Code/edit/master/RaytracingSamples)
基于光追管线实现的程序，主要是一些光追框架使用示例，以及一些基于光追管线的Demo。<br>
