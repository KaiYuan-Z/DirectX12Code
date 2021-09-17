# 光栅管线的Demos

## 1. D3D12_Framework
* Win10 SDK 版本：10.0.17763.0或以上
* DirectX 版本：12
* 是否开启光追：否
* 引用Lib：Core
* 用途：GraphicFramework基本使用流程的演示Demo
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/D3D12_Framework.png" width="960"/></div>

<br>

<br>

## 2. DeferredModelView
* Win10 SDK 版本：10.0.17763.0或以上
* DirectX 版本：12
* 是否开启光追：否
* 引用Lib：Core、Effects
* 用途：验证GraphicFramework的场景配置和Deferred Blinn Phong Shading的Demo
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/DeferredModelView.png" width="960"/></div>

<br>

<br>

## 3. DeferredPbrModelView
* Win10 SDK 版本：10.0.17763.0或以上
* DirectX 版本：12
* 是否开启光追：否
* 引用Lib：Core、Effects
* 用途：验证Deferred PBR Shading的Demo
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/DeferredPbrModelView.png" width="960"/></div>

<br>

<br>

## 4. PbrModelViewer
* Win10 SDK 版本：10.0.17763.0或以上
* DirectX 版本：12
* 是否开启光追：否
* 引用Lib：Core
* 用途：验证Forward PBR Shading的Demo
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/PbrModelViewer.png" width="960"/></div>

<br>

<br>

## 5. SSAOTest(GFSDF_HBAO_Plus)
* Win10 SDK 版本：10.0.17763.0或以上
* DirectX 版本：12
* 是否开启光追：否
* 引用Lib：Core、Effects
* 用途：验证GFSDF_HBAO_Plus是否接入成功的Demo
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/SSAOTest(GFSDF_HBAO_Plus).png" width="960"/></div>

<br>

<br>

## 6. ModelViewer(SimpleSceneEditor)
* Win10 SDK 版本：10.0.17763.0或以上
* DirectX 版本：12
* 是否开启光追：否
* 引用Lib：Core、Effects
* 用途：简易场景编辑器
* 说明：<br>
（1）对于一个复杂场景来说，通过手写配置文件来布置场景实在太困难了，因此花了2小时为自己写了个非常简易的场景编辑器。<br>
（2）考虑到实现鼠标拖拽的话比较麻烦，短时间不好搞定，所以这个简易编辑器只能通过文本框和"+，-"按钮来控制Instance的移动和旋转。<br>
（3）目前编辑器支持配置Model Instance和Grass Instance，且要配置的Model需要提前加到配置文件中，才能在程序中进行配置。<br>
（4）场景编辑完毕后点击Save即可将配置保存到Scene.txt中。其他配置细节说明参见截图：<br>
<div align=center><img src="https://github.com/KaiYuan-Z/DirectX12Code/blob/master/ReadMeImage/ModelViewer(SimpleSceneEditor).png" width="960"/></div>
