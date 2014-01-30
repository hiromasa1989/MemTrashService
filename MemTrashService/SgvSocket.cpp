#include <winsock2.h>
#include <errno.h>
#include "binary.h"
#include "SgvSocket.h"

namespace sigverse
{
	SgvSocket::SgvSocket()
	{
		m_sock = NULL;

		// ログファクトリへの登録
		//Sgv::LogFactory::setLog(1, Sgv::LogPtr(new Sgv::Log(Sgv::Log::DEBUG, "main", 
		//	Sgv::LogFileWriterAutoPtr(new Sgv::DelayFileWriter<Sgv::FileLock>("SgvSocket.log"))))
		//	);
		//mLog = Sgv::LogFactory::getLog(1);
	}

	SgvSocket::SgvSocket(SOCKET sock)
	{
		m_sock = sock;
	}


	SgvSocket::~SgvSocket()
	{
		closesocket(m_sock);
		//WSACleanup();
	}

	bool SgvSocket::initWinsock()
	{

		WSADATA wsaData;

		int err = WSAStartup(MAKEWORD(2,2), &wsaData);
		if (err != 0)
		{
			switch (err) {
				case WSASYSNOTREADY:
					{
						//mLog->err("WSASYSNOTREADY\n");
						break;
					}
				case WSAVERNOTSUPPORTED:
					{
						//mLog->err("WSAVERNOTSUPPORTED\n");
						break;
					}
				case WSAEINPROGRESS:
					{
						//mLog->err("WSAEINPROGRESS\n");
						break;
					}
				case WSAEPROCLIM:
					{
						//mLog->err("WSAEPROCLIM\n");
						break;
					}
				case WSAEFAULT:
					{
						//mLog->err("WSAEFAULT\n");
						break;
					}
			}
			WSACleanup();
			return false;
		}

		//log->flush();			 
		return true;
	}

	bool SgvSocket::connectTo(const char *hostname, int port)
	{
		// ログポインタ取得
		//Sgv::LogPtr log = Sgv::LogFactory::getLog(0);

		struct sockaddr_in server;
		//char buf[32];
		unsigned int **addrptr;

		// TCP指定
		m_sock = socket(AF_INET, SOCK_STREAM, 0);

		server.sin_family = AF_INET;
		server.sin_port = htons(port);

		// ホスト名(もしくはip)の設定
		server.sin_addr.S_un.S_addr = inet_addr(hostname);
		if (server.sin_addr.S_un.S_addr == 0xffffffff) {
			struct hostent *host;

			host = gethostbyname(hostname);
			// ホスト名からホストが見つからなかった場合
			if (host == NULL) {
				if(WSAGetLastError() == WSAHOST_NOT_FOUND) {
					MessageBox(NULL, "Could not find host.", "error", MB_OK);
					//mLog->printf(Sgv::LogBase::ERR,"host not found : %s\n", hostname);
				}
				return false;
			}

			addrptr = (unsigned int **)host->h_addr_list;

			while (*addrptr != NULL) {
				server.sin_addr.S_un.S_addr = *(*addrptr);

				// connect()が成功したらloopを抜けます
				if (connect(m_sock,
					(struct sockaddr *)&server,
					sizeof(server)) == 0) {
						break;
				}

				addrptr++;
				// connectが失敗したら次のアドレスで試します
			}

			// connectが全て失敗した場合
			if (*addrptr == NULL) {
				//MessageBox(NULL, "Could not connect", "error", MB_OK);
				//mLog->printf(Sgv::LogBase::ERR, "connect failed: %d\n", WSAGetLastError());
				return false;
			}
		}
		else
		{
			// inet_addr()が成功したとき

			// connectが失敗したらエラーを表示して終了
			if (connect(m_sock,
				(struct sockaddr *)&server,
				sizeof(server)) != 0) {
					//MessageBox(NULL, "Could not connect", "error", MB_OK);
					//mLog->printf(Sgv::LogBase::ERR, "connect : %d\n", WSAGetLastError());
					//mLog->flush();
					return false;
			}
		}

		return true;
	}

	bool SgvSocket::sendData( const char *msg, int size)
	{
		int sended = 0;

		// 一度にすべて送信できない場合に対応
		while(1){

			// 送信
			int r = send(m_sock, msg + sended, size - sended, 0);

			// 送信失敗
			if(r < 0)
			{
				if(errno == EAGAIN || errno == WSAEWOULDBLOCK){
					Sleep(1);
					continue;
				}
				else{
					return false;
				}
			}
			// すでに送ったバイト数
			sended += r;

			// 全部送信完了したかチェック
			if(size == sended) break;

		}
		return true;
	}

	bool SgvSocket::recvData( char *msg, int size)
	{
		int recieved = 0;

		// 一度にすべて送信できない場合に対応
		while(1){

			// 受信
			int r = recv(m_sock, msg + recieved, size - recieved, 0);

			// 受信失敗
			if(r < 0)
			{
				if(errno == EAGAIN || errno == WSAEWOULDBLOCK){
					Sleep(1);
					continue;
				}
				else{
					return false;
				}
			}
			// すでに受信したバイト数
			recieved += r;

			// 全部受信したかチェック
			if(size == recieved) break;

		}
		return true;
	}

}
