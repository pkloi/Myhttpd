//快速入门管道
//使用匿名管道来实现通信

#include <stdio.h>
#include <Windows.h>

int main(void)
{
	//创建管道
	HANDLE pipe[2];//管道两边的句柄

	//管道的属性
	SECURITY_ATTRIBUTES pipeAtt;
	pipeAtt.nLength = sizeof(pipeAtt);
	pipeAtt.bInheritHandle = true;//创建的子进程能否继承父进程
	pipeAtt.lpSecurityDescriptor = 0;//安全描述符，0为默认值


	//参数1：管道读端;参数2：管道写端;参数3：管道属性;参数4：
	bool bCreate = CreatePipe(&pipe[0], &pipe[1], &pipeAtt, 0);
	if (bCreate == false)
	{
		MessageBox(0, "创建CGI管道失败！", 0, 0);
		return 1;
	}

	//创建进程（在Windows要指定执行什么命令/程序）
	char cCmd[] = "ping www.baidu.com";
	
	//子进程的启动属性
	STARTUPINFO startinfo = { 0 };
	startinfo.cb = sizeof(startinfo);
	startinfo.hStdOutput = pipe[1];//把子线程的标准输出重定向到管道的写端
	//使用显示窗口，使用标准输入句柄
	startinfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	PROCESS_INFORMATION LPriInfo;
	bCreate = CreateProcess(NULL, cCmd, 0, 0, true, 0, 0, 0, &startinfo, &LPriInfo);
	if (!bCreate)
	{
		MessageBox(0, "子进程创建失败！", 0, 0);
		return 1;
	}

	char cBuff[1024];
	DWORD dwSize;
	while (1)
	{
		//如果写入的和读出的内容一样，则表明管道为通的
// 		printf("请输入：");
// 		gets_s(cBuff, sizeof(cBuff));
// 
// 		WriteFile(pipe[1], cBuff, strlen(cBuff) + 1, &dwSize, NULL);
// 		printf("已经写入了%d个字节\n", dwSize);

		ReadFile(pipe[0], cBuff, sizeof(cBuff), &dwSize, NULL);

		cBuff[dwSize] = '\0';
		printf("已经读了%d个字节：[%s]\n", dwSize,cBuff);
	}

	return 0;
}