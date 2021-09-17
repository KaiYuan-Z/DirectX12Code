# GraphicFramework

## 说明：
* 虽然现在已经存在不少比较好用的实验用渲染框架（比如：Falcor等），但是为能更进一步理解DX12和DXR相关的渲染框架设计，所以还是参考着github上成熟的框架自己搭了一个。
* 搭建Graphic Framework过程中，并没有只参考某一个框架，而是根据实际需求从每个成熟框架中借鉴一些自己真正需要的部分，并根据自己的积累和思考加入了一些自己的设计和修改。
* 原本每个模块的编写心得和思考都穿插在各个代码文件的注释里，为了便于查看现已统一整理到各个模块的README中，点击目录中的标题便可查看。

<br>

<br>

## 目录：
### [Core](https://github.com/KaiYuan-Z/DirectX12Code/tree/master/GraphicFramework/Core)：核心框架的Lib
### [RaytracingUtility](https://github.com/KaiYuan-Z/DirectX12Code/tree/master/GraphicFramework/RaytracingUtility)：提供光追支持的Lib
### [Effects](https://github.com/KaiYuan-Z/DirectX12Code/tree/master/GraphicFramework/Effects)：Effect的Lib（将各种常见的图形算法封装成一个个Effect，以便于在各个Demo程序间复用）
### [Tools](https://github.com/KaiYuan-Z/DirectX12Code/tree/master/GraphicFramework/Tools)：各种工具程序的代码（包括：*.h3d模型导出工具）
