#include <Arduino.h>

/*
 * 此程序是鱼缸灯控制器
 * 可定时开关灯(粒度：整点)及 手动开关, 并且默认凌晨2点强制关灯
 * 可调整开关灯时间（输入小于12，则改变开灯时间，反之为关灯时间）
 */
#define BLINKER_PRINT Serial

#define BLINKER_WIFI         //MQTT协议所需要的宏定义 写上就对了不用关注

#include <Blinker.h>         //blinker的头文件调用
#include <String.h>

//char auth[] = "008b3efdc7df";   //在双引号里把*们换 成你在app上选择WiFi链接出来的内一串key
char auth[] = "a4f0faa93656";   //在双引号里把*们换 成你在app上选择WiFi链接出来的内一串key
char ssid[] = "CMCC-57Th";       //在双引号换成写你家WiFi名称/手机热点名称
char pswd[] = "kc9dhxz6";    //双引号换成写你要连接的wifi密码

// 新建组件对象
BlinkerButton Button1("btn-abc");        //button是按键的意思  这句是新建一个按键型组件名叫btn-abc
BlinkerButton Button2("btn-abc2");      //设备状态灯
BlinkerNumber Number1("num-abc");        //这句是新建一个数字组件名字交num-abc
BlinkerText Text1("tex-1");             //显示鱼缸灯on/off，以及设定的自动开关灯时间

int counter = 0;//操作计数

int lamp = 0;//鱼缸灯开关状态（0，1）
int lamp_control = -1;//鱼缸灯 手动开关(-1:自动关闭,0:手动关, 1：开)

int start_hour = 11;//默认开灯时间
int end_hour = 20;//默认关灯时间
int forced_off_time = 2;//强制关灯时间
int input_hour = 0;//手动输入时间，用于改变自动开关灯时间（输入小于12，则改变开灯时间，反之为关灯时间）

int old_hour = -1;//旧时间记录，用于判断小时数是否改变，触发开关灯判断逻辑（1~24:自然时间 -1:设备初始状态）

// 按下按键即会执行该函数 鱼缸灯开关处理
void button1_callback(const String & state)
{
    lamp_control = lamp == 0 ? 1 : 0;
    lamp = lamp_control;
    digitalWrite(0, !digitalRead(0));
    String lamp_status = (lamp == 0 ? "OFF" : "ON");
    char flag[22];
    String time_str = strcat(itoa(input_hour, flag, 10), "点");
    String time_info = "最后输入时间：" + time_str;
    Number1.color("#FF0000");
    Text1.color("#FF0000");
    if(lamp == 1){
        Button1.color("#FF0000");
    }
    Text1.print(lamp_status, time_info);
}

// 按下按键即会执行该函数
void button2_callback(const String & state)                //理解成这是一个中断调用函数 state 是调用中断后手机app发来的值
{
    BLINKER_LOG("get button state: ", state);              //在blinker和Monitor(监视器的意思)上显示这一坨
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  //按一次小灯状态取反一次 亮就变成灭 灭就变成亮
}

// 如果未绑定的组件被触发，则会执行其中内容
void dataRead(const String & data)                          //如果没定义过的组件没触发了话 就是我们之前不是定义了俩组件 其余的组件被触发了比如按钮被按了
{
    char flag[2],a[2],b[2];//数字转字符串
    String time_info;//加时信息
    BLINKER_LOG("Blinker readString: ", data);  //同上面内个
    counter++;                                  //emmmm这句就算了不解释了
    Number1.print(counter);                     //counter的数值打印在(就是显示在)blinker的我们定义过的数字组件上


    String time_str = strcat(itoa(input_hour, flag, 10), "点");
    if(isdigit(data[0])){
        input_hour = atoi(data.c_str());
        time_str = strcat(itoa(input_hour, flag, 10), "点");
        time_info = "刚输入时间：" + time_str;
        //调整开关灯时间（输入小于12，则改变开灯时间，反之为关灯时间）
        if(input_hour <= 12){
            start_hour = input_hour;
        }else{
            end_hour = input_hour;
        }
        old_hour = -1;
    }else{
        time_info = "最后输入时间：" + time_str;
    }

    String lamp_status = lamp == 0 ? "OFF" : "ON";

    char remark[22] = " (";
    strcat(remark, itoa(start_hour, a, 10));
    strcat(remark, "~");
    strcat(remark, itoa(end_hour, b, 10));
    strcat(remark, "点)");

    Text1.print(lamp_status + remark, time_info);
}

void setup()
{
    // 初始化串口
    Serial.begin(115200);
    BLINKER_DEBUG.stream(Serial);     //打开blinker调试函数 debug(调试)就是你发送的信息啥啥的正不正常 否则你打开串口 会显示error

    // 初始化有LED的IO
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);


    // 初始化开关断路器
    pinMode(0, OUTPUT);
    digitalWrite(0, HIGH);

    // 初始化blinker
    Blinker.begin(auth, ssid, pswd);  //链接wifi 到对应的API key上  就是联网然后到你手机界面用的
    Blinker.attachData(dataRead);     //初始化中断 如果被触发 调用dataRead函数  触发条件:没有定义过的按键被按了先这吗理解

    Button1.attach(button1_callback);//初始化button1按键的中断 如果button1被按了话 就调用括号里的函数
    Button2.attach(button2_callback);
}

void loop() {
    Blinker.run();  //blinkr驱动开始不断的接受值和发送值
    int8_t hour = Blinker.hour();
    if(old_hour != hour){//新的小时数才能进入代码块
        old_hour = hour;
        if(hour >= start_hour && hour < end_hour){
            if(hour == start_hour){
                lamp_control = -1;
            }
            if(lamp_control == -1 && lamp == 0){
                lamp = 1;
                digitalWrite(0, LOW);//开
            }
        }else{
            if(hour == end_hour){
                lamp_control = -1;
            }
            if(lamp_control == -1 && lamp == 1){
                lamp = 0;
                digitalWrite(0, HIGH);//关
            }

        }
        if(hour == forced_off_time){//凌晨2点获取开关权限（强制关灯）
            lamp_control = -1;
        }
    }

//    Blinker.delay(1000); //延时1秒
}
