/*****************************************************************************
* @文件名 main.cpp
* @作者 : pkloi
* @日期 : 2023-8-13 16:38:04
* @输入 : 
* @输出 : 
* @上一次修改 : 
* @描述 :C语言http网站服务器
* @作用 :
* 
*****************************************************************************/

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

//网络通信需要包含的头文件和需要加载的库文件
#include <WinSock2.h>
#pragma comment(lib,"WS2_32.lib")

//__FUNCTION__:当前函数名
#define PRINTF(str) printf("[%s - %d]"#str" = %s\n",__FUNCTION__,__LINE__,str);

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

#pragma region 设置套接字属性，端口可复用性
	//SOL_SOCKET:设置套接字的属性使用
	//SO_REUSEADDR:使端口可以重复使用
	int nOpt = 1;//是否可重复使用的值，1为可重复使用
	nRet = setsockopt(nServer_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&nOpt, sizeof(nOpt));
	if (nRet == -1)
	{
		error_die("端口可复用");
	}
#pragma endregion

#pragma region 绑定套接字和网络地址
	//配置服务器端的网络地址
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));//清空设为0
	server_addr.sin_family = PF_INET;//网络地址类型 PF_INET=>internetwork: UDP, TCP, etc.
	server_addr.sin_port = htons(*nPort);//直接取端口可能有问题，需要转一下  htons(u_short)：本机转网络端 
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址 INADDR_ANY：允许任何人访问

	//绑定套接字
	nRet = bind(nServer_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (nRet < 0)
	{
		error_die("sockt bind");
	}

#pragma endregion

#pragma region 动态分配一个端口
	int nNameLen = sizeof(server_addr);
	if (*nPort == 0)//为0时才重新分配
	{
		/*getsockname:返回参数sockfd指定的本地IP和端口，
		*				当套接字的地址与INADDR_ANY绑定时,除非使用connect或accept，否则函数将不返回本地IP的任何信息，
		*				但是端口号可以返回
		*/
		nRet = getsockname(nServer_sock, (struct sockaddr*)&server_addr, &nNameLen);
		if (nRet < 0)
		{
			error_die("getsockname");
		}

		*nPort = server_addr.sin_port;//getsockname获取到的ip和地址都会存入server_addr中
	}
#pragma  endregion

#pragma region 创建监听地址
	nRet = listen(nServer_sock, 5);//设置长度为5的监听队列
	if (nRet < 0)
	{
		error_die("listen");
	}
#pragma endregion
	return nServer_sock;
}

//从指定的客户端套接字，读取一行数据，保存到buff
//返回实际读取到的字节数
int get_line(int nSocket, char* buff, int nSize)
{
	char c = 0;//'\0'
	int i = 0;

	while (i < nSize - 1 && c != '\n')
	{
		int n = recv(nSocket, &c, 1, 0);
		if (n > 0)
		{
			// \r\n  \r是回车，\n是换行
			if (c == '\r')
			{
				//通常recv()函数的最后一个参数为0,代表从缓冲区取走数据,
				//而当为MSG_PEEK时代表只是查看数据,而不取走数据
				n = recv(nSocket, &c, 1, MSG_PEEK);
				if (n > 0 && c == '\n')//正常情况下是\r\n，但不保证意外，需要判一下
				{
					recv(nSocket, &c, 1, 0);//是'\n'正常继续读
				}
				else
				{
					c = '\n';
				}
			}

			buff[i++] = c;
		}
		else
		{
			c = '\n';
		}
	}

	buff[i] = 0;//'\n'

	return i;
}

//向指定的套接字，发送一个提示还没有实现的错误页面
void unimplement(int nClient)
{
	//to do
}

//网页不存在
void not_found(int nClient)
{
	char cBuff[1024];

	//状态行：版本+空格+状态码+短语+回车换行，比如："HTTP/1.0 200 OK\r\n"
	strcpy_s(cBuff, "HTTP/1.0 404 NOT FOUND\r\n");
	send(nClient, cBuff, strlen(cBuff), 0);

	//Server：服务器端的软件名称+版本号
	strcpy_s(cBuff, "Server: AbcHttpd/1.1\r\n");
	send(nClient, cBuff, strlen(cBuff), 0);

	strcpy_s(cBuff, "Content-type:text/html; charset=utf-8\n");
	send(nClient, cBuff, strlen(cBuff), 0);

	strcpy_s(cBuff, "\r\n");
	send(nClient, cBuff, strlen(cBuff), 0);

	//发送404网页内容
	sprintf(cBuff, "<html>							\
			<title>Not Found</title>		\
			<head>						\
				<meta charset=\"utf - 8\">		\
			</head>						 \
			<body>						 \
					<h1>404 找不到页面！</h1>		\
					<p>页面可能走丢了...</p>	   \
				<img src=\"NotFound.jpg\"/>		  \
			</body>								  \
		</html>");
	send(nClient, cBuff, strlen(cBuff), 0);

}

//发送响应包的头信息
void headers(int nClient, const char* cFileType)
{
	char cBuff[1024];

	//状态行：版本+空格+状态码+短语+回车换行，比如："HTTP/1.0 200 OK\r\n"
	strcpy_s(cBuff, "HTTP/1.0 200 OK\r\n");
	send(nClient, cBuff, strlen(cBuff), 0);

	//Server：服务器端的软件名称+版本号
	strcpy_s(cBuff, "Server: AbcHttpd/1.1\r\n");
	send(nClient, cBuff, strlen(cBuff), 0);

	if (!stricmp(cFileType,".html"))
	{
		strcpy_s(cBuff, "Content-type:text/html; charset=utf-8\n");
	}
	else if (!stricmp(cFileType, ".jpeg") || !stricmp(cFileType, ".jpg"))
	{
		strcpy_s(cBuff, "Content-Type: image/jpeg\n");
	}
	else if (!stricmp(cFileType, ".ZIP"))
	{
		strcpy_s(cBuff, "Content-Type: application/zip\n");
	}
	else if (!stricmp(cFileType, ".pdf"))
	{
		strcpy_s(cBuff, "Content-Type: application/pdf\n");
	}
	else if (!stricmp(cFileType, ".xml"))
	{
		strcpy_s(cBuff, "Content-type: text/xml\n");
	}
	send(nClient, cBuff, strlen(cBuff), 0);

	strcpy_s(cBuff, "\r\n");
	send(nClient, cBuff, strlen(cBuff), 0);

}

void cat(int nClient, FILE* resource)
{
	char cBuff[4096];
	int nCount = 0;

	while (1)
	{
		int nRet = fread(cBuff, sizeof(char), sizeof(cBuff), resource);
		if (nRet <= 0)
		{
			break;
		}
		send(nClient, cBuff, nRet, 0);
		nCount += nRet;
	}
	printf("一共发送[%d]字节给浏览器！\n", nCount);
}

//把文件从服务器发出去
void server_file(int nClient, const char* fileName)
{
	int nNumberChars = 1;
	char cBuff[1024];

	//请求包的剩余数据读取完毕
	while (nNumberChars > 0 && strcmp(cBuff, "\n"))
	{
		nNumberChars = get_line(nClient, cBuff, sizeof(cBuff));
		if (strlen(cBuff) - 1 > 0)
		{
			PRINTF(cBuff);
		}
	}

	FILE* resource = NULL;

	const char* cFileType = strrchr(fileName, '.');
	if (strcmp(cFileType, ".html") == 0)
	{
		resource = fopen(fileName, "r");
	}
	else
	{
		resource = fopen(fileName, "rb");
	}
	if (resource == NULL)
	{
		not_found(nClient);
	}
	else
	{
		//正式发送资源给浏览器
		headers(nClient, cFileType);

		//发送请求的资源信息
		cat(nClient, resource);

		printf("资源发送完毕！\n");

		//输出当前时间
		SYSTEMTIME currentTime;
		GetSystemTime(&currentTime);

		printf("当前时间: %u/%u/%u %u:%u:%u:%u %d\n",
			currentTime.wYear, currentTime.wMonth, currentTime.wDay,
			currentTime.wHour, currentTime.wMinute, currentTime.wSecond,
			currentTime.wMilliseconds, currentTime.wDayOfWeek);
	}
	fclose(resource);
}

//处理用户请求的线程函数
DWORD WINAPI accept_request(LPVOID arg)//LPVOID == void*  arg=>套接字
{
	/*数据包格式：
	*	GET / HTTP/1.1\n
	*	Host: 127.0.0.1:8000\n
	*	Connection: keep-alive\n
	*	Cache-Control: max-age=0\n
	*	Upgrade-Insecure-Requests: 1\n
	*	User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.198 Safari/537.36\n
	*	Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,* \ 
	*	/*; q = 0.8, application / signed - exchange; v = b3; q = 0.9\n
	*	Sec - Fetch - Site: none\n
	*	Sec - Fetch - Mode: navigate\n
	*	Sec - Fetch - User: ? 1\n
	*	Sec - Fetch - Dest: document\n
	*	Accept - Encoding: gzip, deflate, br\n
	*	Accept - Language: zh - CN, zh; q = 0.9\n
	*	\n
	*/

	//解析
	char cBuff[1024];//1K

	int nClient = (SOCKET)arg;//客户端套接字

	//读一行数据
	int nNumberChars = get_line(nClient, cBuff, sizeof(cBuff));

	//如果在服务器打开的时候关闭浏览器strlen(cBuff)==0,会报错
	if (strlen(cBuff) <= 0 || strlen(cBuff) >= 1024)
	{
		not_found(nClient);
	}
	PRINTF(cBuff);
	

#pragma region 获取提交方法 GET POST
	char cMethod[255];
	int i = 0, j = 0;
	while (!isspace(cBuff[j]) && i < sizeof(cMethod) - 1)
	{
		cMethod[i++] = cBuff[j++];
	}
	cMethod[i] = 0;//'\n'
	PRINTF(cMethod);

	//检查提交的方法服务器是否支持
	if (_stricmp(cMethod, "GET") && _stricmp(cMethod, "POST"))//_stricmp 比较时不区分大小写
	{
		//向浏览器返回一个错误页面
		//to do 
		unimplement(nClient);
	}
#pragma endregion
	
#pragma region 解析资源文件的路径
	//"GET /test/abc.html HTTP/1.1\n"
	char cURL[255];//存放请求的资源的完整路径
	i = 0;
	//跳过空格
	while (isspace(cBuff[j]) && j < sizeof(cBuff))
	{
		j++;
	}

	while (!isspace(cBuff[j]) && i < sizeof(cURL) - 1 && j < sizeof(cBuff))
	{
		cURL[i++] = cBuff[j++];
	}
	cURL[i] = 0;
	PRINTF(cURL);

	//获取资源的完整路径

	//www.abc.com
	//127.0.0.1
	//url /
	//htdocs/index.html

	char cPath[512] = "";
	sprintf(cPath, "htdocs%s", cURL);
	//如果最后一个字符为'/'，则默认访问index.tml，否则直接访问指定的资源路径
	if (cPath[strlen(cPath) - 1] == '/')
	{
		strcat(cPath, "index.html");
	}
	PRINTF(cPath);
#pragma endregion

	//判断访问的是文件还是目录
	struct stat status;
	int nRet = stat(cPath, &status);
	if (nRet == -1)
	{
		//请求包的剩余数据读取完毕
		while (nNumberChars > 0 && strcmp(cBuff, "\n"))
		{
			nNumberChars = get_line(nClient, cBuff, sizeof(cBuff));
		}
		not_found(nClient);
	}
	else
	{
		//使用位操作&，检查类型
		if ((status.st_mode & S_IFMT) == S_IFDIR)//S_IFMT:File type mask
		{
			//如果为目录，则添加html文件
			strcat(cPath, "/index.html");
		}

		server_file(nClient, cPath);//把文件发出去
	}

	closesocket(nClient);
	return 0;
}

int main(void)
{
	unsigned short nPort = 8000;
	int nServer_sock = startUp(&nPort);

	printf("httpd服务已经启动，正在监听 %d 端口...\n", nPort);

	//让外部允许访问
	struct sockaddr_in client_addr;//客户端
	int nClientLen = sizeof(client_addr);

	while (1)
	{
		//阻塞式等待用户通过浏览器发起访问
		//有访问后，生成一个新的套接字
		int nClient_sock = accept(nServer_sock, (struct sockaddr*)&client_addr, &nClientLen);//获取客户端的信息
		if (nClient_sock == -1)
		{
			error_die("accept");
		}

		//创建线程
		DWORD dwThreadID = 0;//线程唯一ID
		//accept_request函数的参数为第三个参数nClient_sock传的值
		CreateThread(0, 0, accept_request, (void*)nClient_sock, 0, &dwThreadID);
	}

	closesocket(nServer_sock);
	return 0;
}