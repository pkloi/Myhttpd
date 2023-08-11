/*****************************************************************************
* @文件名 main.cpp
* @作者 : pkloi
* @日期 : 2023/8/11 21:42
* @输入 : 
* @输出 : 
* @上一次修改 : 
* @描述 :C语言http网站服务器
* @作用 :
* 
*****************************************************************************/

#include <stdio.h>
#include <string.h>

//网络通信需要包含的头文件和需要加载的库文件
#include <WinSock2.h>
#pragma comment(lib,"WS2_32.lib")

//错误信息提示
void error_die(const char* str)
{
	char* cError = const_cast<char*>(str);
	char* cText=const_cast<char*>(" 错误，程序启动失败！");

	char ErrorText[255];

	strcpy_s(ErrorText, cError);
	strcat_s(ErrorText, cText);

	perror(ErrorText);
	exit(1);
}

//实现网络的初始化
//返回值：套接字
//参数：nPort 端口；
//nPort：如果*nPort为0，则自动分配一个可用的端口（Tinyhttpd）
int startUp(unsigned short *nPort)
{
	/*流程：
	* 1、网络通信初始化（Linux系统不需要初始化，Windows需要）
	* 2、创建套接字
	* 3、设置套接字属性，端口可复用性（该步骤可略）
	* 4、绑定套接字和网络地址
	* 5、动态分配端口号（该步骤可略）
	*/

#pragma region 网络通信初始化

	/*WSAStartup结构：
	int WSAStartup(
	_In_ WORD wVersionRequested,//协议版本
	_Out_ LPWSADATA lpWSAData
	);
	*/
	WSAData data;//一般不需要访问，但是初始化需要，把内部初始化信息保存进去
	int nRet = WSAStartup(
		MAKEWORD(1, 1),//一个1.1的版本协议，第一个参数为版本号，第二个参数为子版本号
		&data);//nRet==1 失败;nRet==0 成功;

	if (nRet)
	{
		error_die("WSAStartup");
	}

#pragma endregion

#pragma region 创建套接字
	/*SOCK_STREAM:	表示的是字节流，对应 TCP
	* SOCK_DGRAM：	表示的是数据报，对应 UDP
	* SOCK_RAW:		表示的是原始套接字
	*/
	int nServer_sock = socket(PF_INET,//套接字的类型 PF_INET：网络套接字
		SOCK_STREAM,//数据流
		IPPROTO_TCP);

	//检查是否创建成功
	if (nServer_sock == -1)
	{
		error_die("套接字");
	}

#pragma endregion

#pragma region 设置套接字属性

#pragma endregion

#pragma region 网络通信初始化

#pragma endregion

#pragma region 绑定套接字和网络地址

#pragma endregion
	return -1;
}

int main(void)
{
	unsigned short nPort = 80;
	int nServer_sock = startUp(&nPort);

	printf("httpd服务已经启动，正在监听 %d 端口...\n", nPort);

}