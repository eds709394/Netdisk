#include "StdAfx.h"
#include "Client.h"
#include "locale.h"
#include "NetdiskClient.h"
#include "NetdiskClientDlg.h"

Client::Client(void)
{
	m_pMainApp=(CNetdiskClientApp*)AfxGetApp() ;
}


Client::~Client(void)
{
}

bool Client::ConnectToServ(SOCKET *pSocket,CString Serveraddr,int port)
{
	struct sockaddr_in ServerAddress;
	struct hostent *Server;

	// 生成SOCKET
	*pSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == *pSocket) 
	{
		TRACE("错误：初始化Socket失败，错误信息：%d\n", WSAGetLastError());
		return false;
	}

	//获取系统默认的发送数据缓冲区大小
	unsigned int uiRcvBuf;
	int uiRcvBufLen = sizeof(uiRcvBuf);
	int nErrCode= getsockopt(m_Client.sock, SOL_SOCKET, SO_SNDBUF,(char*)&uiRcvBuf, &uiRcvBufLen);
	if (SOCKET_ERROR == nErrCode)
	{
		return false;
	}

	//设置系统发送数据缓冲区为默认值的BUF_TIMES倍
	uiRcvBuf *= 10;
	nErrCode = setsockopt(m_Client.sock, SOL_SOCKET, SO_SNDBUF,(char*)&uiRcvBuf, uiRcvBufLen);
	if (SOCKET_ERROR == nErrCode)
	{
		AfxMessageBox(_T("修改系统发送数据缓冲区失败！"));
	}


	//检查设置系统发送数据缓冲区是否成功
	unsigned int uiNewRcvBuf;
	getsockopt(m_Client.sock, SOL_SOCKET, SO_SNDBUF,(char*)&uiNewRcvBuf, &uiRcvBufLen);
	if (SOCKET_ERROR == nErrCode || uiNewRcvBuf != uiRcvBuf)
	{
		AfxMessageBox(_T("修改系统发送数据缓冲区失败！"));;
	}

	char* pAddr;
	int len = Serveraddr.GetLength(); 
	pAddr = (char*)malloc((len*2+1)*sizeof(char));
	//CString的长度中汉字算一个长度 
	memset(pAddr, 0, 2*len+1); 
	USES_CONVERSION; 
	strcpy((LPSTR)pAddr,OLE2A(Serveraddr.LockBuffer()));

	// 生成地址信息
	Server = gethostbyname(pAddr);
	if (Server == NULL) 
	{
		closesocket(*pSocket);
		TRACE("错误：无效的服务器地址.\n");
		return false; 
	}


	ZeroMemory((char *) &ServerAddress, sizeof(ServerAddress));
	ServerAddress.sin_family = AF_INET;
	CopyMemory((char *)&ServerAddress.sin_addr.s_addr, 
		(char *)Server->h_addr,
		Server->h_length);

	ServerAddress.sin_port = htons(port);

	// 开始连接服务器
	if (SOCKET_ERROR == connect(*pSocket, reinterpret_cast<const struct sockaddr *>(&ServerAddress),sizeof(ServerAddress))) 
	{
		closesocket(*pSocket);
		TRACE("错误：连接至服务器失败！\n");
		return false; 
	}

	return true;
}

// 初始化WinSock 2.2
bool Client::LoadSocketLib()
{    
	WSADATA wsaData;
	int nResult = WSAStartup(MAKEWORD(2,2), &wsaData);

	if (NO_ERROR != nResult)
	{
		AfxMessageBox (_T("初始化WinSock 2.2失败！\n"));
		return false; // 错误
	}

	return true;
}

bool Client::Start()
{
	if(!LoadSocketLib())
	{
		AfxMessageBox(_T("加载socket失败！"));
		return false;
	}

	if(!ConnectToServ(&m_Client.sock,_T("127.0.0.1"),12345))
	{
		AfxMessageBox(_T("连接服务器失败！"));
		return false;
	}

	return true;
}

void Client::Stop()
{
	if(m_Client.sock!=NULL)
	{
		closesocket(m_Client.sock);
		m_Client.sock=NULL;
	}

}

void Client::SendMsgToServ(CString strMsg,int sendType)
{
	/*char szTemp[MAX_BUFFER_LEN];*/
	//memset( szTemp,0,sizeof(szTemp) );
	DataPackage sendPack;
	ZeroMemory(&sendPack,sizeof(sendPack));
	
	int nBytesSend=0;
	// 向服务器发送信息
	int n=strMsg.GetLength();
	int len=WideCharToMultiByte(CP_ACP,0,strMsg,strMsg.GetLength(),NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,strMsg,strMsg.GetLength()+1,sendPack.sContent,len+1,NULL,NULL);
	sendPack.sContent[len]='\0';
	sendPack.nContentLen=len;
	sendPack.nPackLen=sizeof(sendPack);
	sendPack.iType=sendType;
	
	nBytesSend = send(m_Client.sock, (char*)&sendPack, sendPack.nPackLen, 0);
//	Sleep(10);
	if (SOCKET_ERROR == nBytesSend) 
	{
		CString tmpStr;
		tmpStr.Format(_T("发送失败，错误代码为:%d"),WSAGetLastError());
		AfxMessageBox(tmpStr);
		return ; 
	}	

}

//接收服务器返回信息
int Client::RecvloginMsg()
{
	int iResult;
	//char recvbuf[MAX_BUFFER_LEN];
	//int recvbuflen=MAX_BUFFER_LEN;
	DataPackage recvPack;
	recvPack.nPackLen=sizeof(recvPack);

	iResult=recv(m_Client.sock,(char*)&recvPack,recvPack.nPackLen,0);
	if(0 == iResult)
	{
		AfxMessageBox(_T("Connection closed\n"));
		return -1;
	}
	if(iResult > 0)
	{
		return atoi(recvPack.sContent);
	}
	else
	{
		AfxMessageBox(_T("recv failed: %d\n"), WSAGetLastError());
		return -1;
	}

}

//清除套接字
void Client::Clean()
{
	if(m_Client.sock != NULL)
	{
		closesocket(m_Client.sock);
		m_Client.sock=NULL;
	}
}

//更新客户端目录信息
bool Client::UpdateClientCatalog(CString Rootpath)
{
	//将用户目录的根目录名发送给服务器，根目录名即为用户登录名
	//SendMsgToServ(m_pMainApp->m_loginName,UPDATECLIENT);
	SendMsgToServ(Rootpath,UPDATECLIENT);	
	//获取返回信息
	if(!RecvReturnMsg())
	{
		AfxMessageBox(_T("获取返回信息失败！"));
		return false;
	}
	return true;
}

//发送目录路径给服务器，服务器返回该目录下列表信息
bool Client::GetCatalogInfo(CString FloderName)
{
	//发送目录路径给服务器 
//	CString sendMsgStr;
	//sendMsgStr.Format(_T("%d+%s"),GETCATALOGINFO,FloderName);
	SendMsgToServ(FloderName,GETCATALOGINFO);

	//获取返回信息
	if(!RecvReturnMsg())
	{
		AfxMessageBox(_T("获取返回信息失败！"));
		return false;
	}
	return true;
}

//发送创建文件夹的信息
bool Client::CreateNewFloder(CString baseFloder)
{
	//发送操作信息到服务器
	SendMsgToServ(baseFloder,NEWFLODER);
	//获取返回信息
	//if(!RecvReturnMsg())
	//{
	//	AfxMessageBox(_T("获取返回信息失败！"));
	//	return false;
	//}
	return true;
}

//发送删除文件信息
bool Client::DeleteClientFile(CString path)
{
	//发送操作信息到服务器
	SendMsgToServ(path,DELETEFILE);
	return true;
}
//发送下载文件信息
bool Client::DownloadFile(CString path)
{
	SendMsgToServ(path,DELETEFILE);
	return true;
}

bool Client::RecvReturnMsg()
{
	//获取用户返回的目录信息
	//CString catalogIndexInfo;
	int iResult;
	//char recvbuf[MAX_BUFFER_LEN];
	//int recvbuflen=MAX_BUFFER_LEN;
	DataPackage recvPack;
	ZeroMemory(&recvPack,sizeof(recvPack));
	
	//CString m_StrIndexInfo;
	//do 
	//{
		iResult=recv(m_Client.sock,(char*)&recvPack,sizeof(recvPack),0);
		Sleep(10);
		if(0 == iResult)
		{
			AfxMessageBox(_T("Connection closed\n"));
			return false;
		}
		//成功获取数据
		if(iResult > 0)
		{
			((CNetdiskClientApp*)AfxGetApp())->m_strIndexInfo=recvPack.sContent;
			return true;
		}
		else
		{
			CString msgStr;
			msgStr.Format(_T("recv failed,错误代码： %d\n"), WSAGetLastError());
			AfxMessageBox(msgStr);
			return false;
		}
}

//刷新客户端信息
bool Client::UpdateClient(CString path)
{
	SendMsgToServ(path,REFRESH);
	return true;
}

//移动文件夹或者文件
bool Client::MoveClientFile(CString path)
{
	SendMsgToServ(path,MOVEFILE);
	return true;
}

//发送更新回收站信息
bool Client::UpdateRecycleList(CString path)
{
	SendMsgToServ(path,RECYCLE_UPDATE);

	//获取返回信息
	if(!RecvReturnMsg())
	{
		AfxMessageBox(_T("获取返回信息失败！"));
		return false;
	}
	return true;
}

//发送回收站还原文件信息
bool Client::RecycleRestore(CString path)
{
	SendMsgToServ(path,RECYCLE_RESTORE);
	return true;
}

//发送删除回收站记录信息
bool Client::RecycleDel(CString path)
{
	SendMsgToServ(path,RECYCLE_DEL);
	return true;
}

//发送清空回收站的信息
bool Client::RecycleClean(CString path)
{
	SendMsgToServ(path,RECYCLE_CLEAN);
	return true;
}

//发送获取文件大小的信息
bool Client::GetFileSizeFromServ(CString path)
{
	SendMsgToServ(path,GETFILESIZEINFO);

	if(!RecvReturnMsg())
		return false;
	return true;
}

//发送历史版本信息
bool Client::GetHistroyVersionInfo(CString strInfo)
{
	SendMsgToServ(strInfo,HISTROYVERSION);
	if(!RecvReturnMsg())
		return false;
	return true;
}

//发送还原历史记录版本信息
bool Client::ReStoreHisVerFile(CString path)
{
	SendMsgToServ(path,HIS_RESTORE);
	return true;
}

//发送删除历史记录版本信息
bool Client::DeleteHisVerFile(CString path)
{
	SendMsgToServ(path,HIS_DELETE);
	return true;
}

//发送清空历史记录版本信息
bool Client::CleanHisVerFile(CString path)
{
	SendMsgToServ(path,HIS_CLEAR);
	return true;
}

//发送查找文件信息
bool Client::FindFileByStr(CString findStr)
{
	SendMsgToServ(findStr,FINDFILEBYSTR);
	if(!RecvReturnMsg())
		return false;
	return true;
}

//发送查找回收站文件信息
bool Client::RecycleFindFile(CString findStr)
{
	SendMsgToServ(findStr,RECYCLE_FIND);
	if(!RecvReturnMsg())
		return false;
	return true;
}

//发送按日期查看文件的信息
bool Client::GetFileByDataShow(CString strInfo)
{
	SendMsgToServ(strInfo,FINDFILEBYTIME);
	if(!RecvReturnMsg())
		return false;
	return true;
}