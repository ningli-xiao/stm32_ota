# mcu# tx_mcu
测试项目：
1.串口，TIM硬件功能 ok
2.AT指令，移远模组 ok
3.mqtt登入，订阅发布 ok
4.mcuTask ok
5.dataProcessTask //如果控制板功能加入此板卡再进行开发
6.看门狗 
7.IO中断重启
8.OTA
9.长时间运行
10.意外情况测试

buglist：
1.publish error
原因：检查发现回复+QMTPUBEX: 0,0,0 冒号后需要加空格，必须严格按照格式来操作。
解决：
2.长时间无操作后模组关机
原因：
解决：



