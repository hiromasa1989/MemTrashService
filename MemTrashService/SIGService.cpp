#include "SIGService.h"
#include "binary.h"

#include <time.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <iostream>
//#include <boost/thread.hpp>
//#include <boost/bind.hpp>

namespace sigverse
{
	std::string IntToString(int x)
	{
		char tmp[32];
		sprintf_s(tmp, 32,"%d",x);
		std::string str = std::string(tmp);
		str += ",";
		return str;
	}

	bool RecvMsgEvent::setData(std::string data, int size)
	{
		// ������                                                                                         
		int strPos1 = 0;
		int strPos2;
		std::string tmpss;

		// ���b�Z�[�W���M���̖��O���擾                                                                   
		strPos2 = data.find(",", strPos1);
		m_from.assign(data, strPos1, strPos2-strPos1);

		strPos1 = strPos2 + 1;

		// ���b�Z�[�W�T�C�Y                                                                               
		strPos2 = data.find(",", strPos1);
		tmpss.assign(data, strPos1, strPos2-strPos1);
		int msgSize = atoi(tmpss.c_str());

		// ���b�Z�[�W�擾                                                                                 
		strPos1 = strPos2 + 1;
		m_msg.assign(data, strPos1, msgSize);

		return true;
	}

	// �R���X�g���N�^
	SIGService::SIGService(std::string name)
	{
		//! �T�[�o�ڑ��p�\�P�b�g
		m_sgvsock = new SgvSocket();
		m_sgvsock->initWinsock();

		m_name = name;
		m_connectedView = false;
		m_viewsock = NULL;
	}

	SIGService::SIGService()
	{
		//! �T�[�o�ڑ��p�\�P�b�g
		m_sgvsock = new SgvSocket();
		m_sgvsock->initWinsock();

		m_connectedView = false;
		m_viewsock = NULL;
	}

	SIGService::~SIGService()
	{
		delete m_sgvsock;

		if(m_viewsock != NULL)
			delete m_viewsock;
	}

	bool SIGService::connect(std::string host, int port)
	{
		if(m_sgvsock->connectTo(host.c_str(), port)) {

			std::string tmp = "SIGMESSAGE," + m_name + ",";
			m_sgvsock->sendData(tmp.c_str(), tmp.size());
		}
		else{
			MessageBox( NULL, "Could not connect to simserver", "Error", MB_OK);
			return false;
		}
		// ���ʂ��󂯎��
		char tmp[8];
		memset(tmp, 0, sizeof(tmp));

		if(recv(m_sgvsock->getSocket(), tmp, sizeof(tmp), 0) < 0) {
			MessageBox( NULL, "Service: Failed to connect server", "Error", MB_OK);
		}

		// �A�^�b�`����
		if(strcmp(tmp,"SUCC") == 0) {
			// �z�X�g���͋L�����Ă���
			m_host = host;
			return true;
		}
		// �T�[�r�X�v���o�C�_�̃A�^�b�`�Ɏ��s
		else if(strcmp(tmp,"FAIL") == 0){
			char tmp[128];
			sprintf_s(tmp, 128, "Service name \"%s\" is already exist.", m_name.c_str());
			MessageBox( NULL, tmp, "Error", MB_OK);
			return false;
		}

		return true;
	}

	bool SIGService::disconnect()
	{
		// �w�b�_�T�C�Y��������-1�ɂȂ�l�𑗂�
		if(!m_sgvsock->sendData("00004", 5)) return false;
		return true;
	}

	bool SIGService::sendMsgToCtr(std::string to, std::string msg)
	{
		std::map<std::string, SgvSocket*>::iterator it;
		it = m_consocks.find(to);

		//�@�R���g���[���ƒ��ڌq�����Ă��邩�ǂ���
		if(it != m_consocks.end()) {
			int msgSize = msg.size();
			std::string ssize = IntToString(msgSize);
			msg[msgSize] = '\0';
			SgvSocket *sock = (*it).second;
			std::string sendMsg = m_name + "," + ssize + msg;
			int tmpsize = sizeof(unsigned short) * 2;
			int sendSize = sendMsg.size() + tmpsize;

			char *sendBuff = new char[sendSize];
			char *p = sendBuff;

			BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0002);
			BINARY_SET_DATA_S_INCR(p, unsigned short, sendSize);
			memcpy(p, sendMsg.c_str(), sendMsg.size());
			if(!sock->sendData(sendBuff, sendSize)) {
				delete [] sendBuff;			
				return false; 
			}

			delete [] sendBuff;	 
			return true;
		}
		// �w�肵���G���e�B�e�B�̃R���g���[���͐ڑ�����Ă��Ȃ�
		else {
			return false;
		}
	}

	bool SIGService::sendMsg(std::string to, std::string msg)
	{
		// ���b�Z�[�W�T�C�Y��ǉ�
		char msize[6];
		int ms = (int)msg.size();
		sprintf_s(msize, 6,"%.5d", ms);
		std::string msg_size;
		msg_size = std::string(msize);

		std::string tmp_data = msg_size + "," + msg + "," + "-1.0," + "1," + to + ",";
		// �f�[�^���M�T�C�Y��擪�ɒǉ�
		char bsize[6];
		int nbyte = (int)tmp_data.size() + 5;
		sprintf_s(bsize, 6,"%.5d", nbyte);
		std::string tmp_size = std::string(bsize);
		tmp_data = tmp_size + tmp_data;
		if(!m_sgvsock->sendData(tmp_data.c_str(), tmp_data.size())) return false;

		return true;
	}

	bool SIGService::sendMsg(std::vector<std::string> to, std::string msg)
	{
		// ���b�Z�[�W�T�C�Y��ǉ�
		char msize[6];
		int ms = (int)msg.size();
		sprintf_s(msize, 6, "%.5d", ms);
		std::string msg_size;
		msg_size = std::string(msize);

		// �ŏ��Ƀ��b�Z�[�W(-1.0�̓��b�Z�[�W���M�͈͖���)
		std::string tmp_data = msg_size + "," + msg + "," + "-1.0,";

		// ���b�Z�[�W���M�G���e�B�e�B�̐�
		int size = to.size();

		// string �ɕϊ�
		char tmp[6];
		sprintf_s(tmp, 6, "%d", size);
		std::string num = std::string(tmp);

		// ���M��G���e�B�e�B�̐��A���O��ǉ�
		tmp_data += num + ","; // + to + "," + msg;
		for(int i = 0; i < size; i++) {
			tmp_data += to[i] + ",";
		}

		// �f�[�^���M�T�C�Y��擪�ɒǉ�
		char bsize[6];
		int nbyte = (int)tmp_data.size() + 5;
		sprintf_s(bsize, 6, "%.5d", nbyte);
		std::string tmp_size = std::string(bsize);
		tmp_data = tmp_size + tmp_data;

		if(!m_sgvsock->sendData(tmp_data.c_str(), tmp_data.size())) return false;
		return true;
	}

	//! ���C�����[�v
	void SIGService::startLoop(double ltime)
	{
		m_start = true;
		static clock_t start;
		clock_t now;
		start = clock();
		while(1){

			// ���[�v�I�����ԂɂȂ�ƏI��
			if(ltime > 0){
				now = clock();
				double tmp =(double)(now - start)/CLOCKS_PER_SEC;
				if(tmp > ltime) break;
			}
			if(!checkRecvData(100)) break;
		}
		m_start = false;
	}

	bool SIGService::checkRecvData(int usec)
	{

		static bool init;
		struct timeval tv;

		// select�ő҂���(100�}�C�N���b)
		tv.tv_sec = 0;
		tv.tv_usec = usec;

		// �T�[�o�ƂȂ����Ă���\�P�b�g
		SOCKET mainSock = m_sgvsock->getSocket();
		if(mainSock == NULL) return false;

		static double timewidth;
		static clock_t start;
		clock_t now;

		// ����I�ɌĂԊ֐�
		if(m_start && !init) {
			onInit();
			timewidth = onAction();
			start = clock();
			init = true;
		}
		else{
			now = clock();
			double tmp =(double)(now - start)/CLOCKS_PER_SEC;

			// onAction�Ăяo�����Ԃɓ��B���Ă���ΌĂяo��
			if(tmp > timewidth) {
				timewidth = onAction();
				start = clock();
			}
		}
		// �t�@�C���f�B�X�N���v�^
		fd_set fds, readfds;

		int n;

		// �ڑ����̃\�P�b�g���X�g
		std::vector<sigverse::SgvSocket*> tmp_socks;
		tmp_socks.push_back(m_sgvsock);

		FD_ZERO(&readfds);

		// �T�[�o�ƂȂ����Ă���\�P�b�g��o�^
		FD_SET(mainSock,&readfds);

		// �r���[���[�ƌq�����Ă���ꍇ
		if(m_connectedView){
			SOCKET viewSock = m_viewsock->getSocket();
			tmp_socks.push_back(m_viewsock);
			// �r���[���[�ƂȂ����Ă���\�P�b�g��o�^
			FD_SET(viewSock,&readfds);
		}

		// �R���g���[���̐ڑ��ς݃\�P�b�g
		std::map<std::string, SgvSocket*>::iterator it;
		it = m_consocks.begin();

		while(it != m_consocks.end()) {
			// �R���g���[���ƂȂ����Ă���\�P�b�g��o�^
			SOCKET sock = (*it).second->getSocket();
			FD_SET(sock,&readfds);
			tmp_socks.push_back((*it).second);
			it++;
		}

		// select��������e���㏑�����Ă��܂��̂ŁA���񏉊������܂�
		memcpy(&fds, &readfds, sizeof(fd_set));

		// fds�ɐݒ肳�ꂽ�\�P�b�g���ǂݍ��݉\�ɂȂ�܂ő҂��܂�
		n = select(0, &fds, NULL, NULL, &tv);

		// �^�C���A�E�g
		if (n == 0) {
			return true;
		}
		// �G���[
		else if(n < 0) {
			return false;
		}

		// �ڑ����̃\�P�b�g�Ń��[�v
		int connectSize = tmp_socks.size();
		for(int i = 0; i < connectSize; i++) {
			// �T�[�o����f�[�^��M���Ă����ꍇ
			SOCKET nowsock = tmp_socks[i]->getSocket();

			// �f�[�^�����Ă����ꍇ
			if (FD_ISSET(nowsock, &fds)) {

				// �w�b�_�A�f�[�^�T�C�Y
				char bsize[4];
				if(!tmp_socks[i]->recvData(bsize,4)) return true;

				// �w�b�_�A�f�[�^�T�C�Y�擾
				char *p = bsize;
				unsigned short header = BINARY_GET_DATA_S_INCR(p, unsigned short);
				unsigned short size = BINARY_GET_DATA_S_INCR(p, unsigned short);

				// ���łɎ擾�����w�b�_������
				size -= 4;

				// �f�[�^�擾
				char *recvBuff = new char[size];
				if(size > 0)
					if(!tmp_socks[i]->recvData(recvBuff, size)) return true;

				// strtok_s�Ŏg��
				char *nexttoken;

				switch(header) {
					// �R���g���[������̃��b�Z�[�W�f�[�^�̏ꍇ
					case 0x0001:
						{
							// ���b�Z�[�W��M�C�x���g�쐬
							RecvMsgEvent msg;
							msg.setData(recvBuff, size);
 
							// ���b�Z�[�W��M�C�x���g�n���h���Ăяo��
							onRecvMsg(msg);
							break;
						}
						// ���̃T�[�r�X���ǉ����ꂽ���Ƃ�m�点��f�[�^�̏ꍇ
					case 0x0002:
						{
							std::string name = recvBuff;
							name[size] = '\0';
							m_elseServices.push_back(name);
							break;
						}
						// �R���g���[������ڑ����N�G�X�g���͂���
					case 0x0003:
						{
							char *pp = recvBuff;
							unsigned short port = BINARY_GET_DATA_S_INCR(pp, unsigned short);


							std::string name = strtok_s(pp, ",", &nexttoken);
							SgvSocket *sock = new SgvSocket();
							sock->initWinsock();

							// �ڑ�����������\�P�b�g�ۑ�(���i�K�ł̓R���g���[���̓T�[�o�Ɠ����z�X�g���Ɖ���j
							if(sock->connectTo(m_host.c_str(), port)) {
								m_consocks.insert(std::map<std::string, SgvSocket*>::value_type(name, sock));
								m_entitiesName.push_back(name);
							}

							char tmp[sizeof(unsigned short)*2];
							char *p = tmp;

							// �ڑ����N�G�X�g��1
							BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0001);
							BINARY_SET_DATA_S_INCR(p, unsigned short, sizeof(unsigned short));

							// �ڑ�������m�点��(�ꉞ)
							sock->sendData(tmp, sizeof(unsigned short)*2);

							break;
						}
						// �ؒf���N�G�X�g
					case 0x0004:
						{
							// �G���e�B�e�B���擾
							char *pp = recvBuff;
							std::string ename = strtok_s(pp, ",", &nexttoken);

							// �R���g���[���Ɛؒf
							disconnectFromController(ename);

							// ���X�g����폜
							std::map<std::string, SgvSocket*>::iterator sockit;
							sockit = m_consocks.find(ename);
							if(sockit != m_consocks.end()) {
								m_consocks.erase(sockit);
							}
							return false;
						}
						// viewer�ؒf���N�G�X�g
					case 0x0005:
						{
							delete m_viewsock;
							m_viewsock = NULL;
							m_connectedView = false;
							break;
						}
				}
				delete [] recvBuff;
			} //if (FD_ISSET(mainSock, &fds)) {
		} // for(int i = 0; i < connectSize; i++){
		return true;
	}

	bool SIGService::connectToViewer()
	{
		//! �r���[���[�ڑ��p�\�P�b�g
		m_viewsock = new SgvSocket();
		//m_viewsock->initWinsock();

		if(!m_viewsock->connectTo("localhost", 11000)) {
			MessageBox( NULL, "Could not connect to viewer", "Error", MB_OK);
			return false;
		}
		// �ڑ�����
		else {
			// �����̃T�[�r�X�v���o�C�_�̖��O�𑗂�
			int nsize = m_name.size();
			int sendSize = nsize + sizeof(unsigned short)*2;
			char *sendBuff = new char[sendSize];
			char *p = sendBuff;

			// �T�[�r�X�v���o�C�_�̖��O���M���N�G�X�g�O
			BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0000);
			BINARY_SET_DATA_S_INCR(p, unsigned short, sendSize);

			// ���M�f�[�^���o�b�t�@�ɃR�s�[
			memcpy(p, m_name.c_str(), m_name.size());
			if(!m_viewsock->sendData(sendBuff, sendSize)) {
				delete [] sendBuff;
			}
			delete [] sendBuff;

			m_connectedView = true;
			return true;
		}
	}
	bool SIGService::disconnectFromController(std::string entityName)
	{
		// �G���e�B�e�B������SgvSocket��T��
		std::map<std::string, SgvSocket*>::iterator it;
		it = m_consocks.find(entityName);

		// ��������
		if(it != m_consocks.end()) {
			// �\�P�b�g�擾
			SgvSocket *sock = (*it).second;

			char tmp[sizeof(unsigned short)*2];
			char *p = tmp;

			// �ؒf���N�G�X�g�𑗂�(����Ԃ�)
			BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0004);
			BINARY_SET_DATA_S_INCR(p, unsigned short, sizeof(unsigned short));

			// ����
			sock->sendData(tmp, sizeof(unsigned short)*2);

			// �G���e�B�e�B��o�^����폜
			m_consocks.erase(it);
			delete sock;

			return true;
		}
		else
			return false;
	}

	void SIGService::disconnectFromAllController()
	{
		// �G���e�B�e�B������SgvSocket��T��
		std::map<std::string, SgvSocket*>::iterator it;
		it = m_consocks.begin();

		// ��������
		while(it != m_consocks.end()) {
			// �\�P�b�g�擾
			SgvSocket *sock = (*it).second;

			char tmp[sizeof(unsigned short)*2];
			char *p = tmp;

			// �ؒf���N�G�X�g�𑗂�(����Ԃ�)
			BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0004);
			BINARY_SET_DATA_S_INCR(p, unsigned short, sizeof(unsigned short));

			// ����
			sock->sendData(tmp, sizeof(unsigned short)*2);

			// �G���e�B�e�B��o�^����폜
			delete sock;
			it++;
		}
		m_consocks.clear();
	}

	void SIGService::disconnectFromViewer()
	{
		// �܂��ڑ����Ă��Ȃ�
		if(!m_connectedView || m_viewsock == NULL) return;

		char buf[4];
		char *p = buf;
		BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0003);
		BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0004);

		m_viewsock->sendData(buf, 4);

		delete m_viewsock;
		m_viewsock = NULL;
	}

	ViewImage* SIGService::captureView(std::string entityName, int camID, ColorBitType ctype,ImageDataSize size)
	{
		if(!m_connectedView){
			MessageBox( NULL, "captureView: Service is not connected to viewer", "Error", MB_OK);
			return NULL;
		}
		std::string cameraID = IntToString(camID);
		std::string camSize = IntToString(size);
		std::string sendMsg = entityName + "," + cameraID + camSize;
		int tmpsize = sizeof(unsigned short) * 2;
		int sendSize = sendMsg.size() + tmpsize;

		char *sendBuff = new char[sendSize];
		char *p = sendBuff;

		BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0001);
		BINARY_SET_DATA_S_INCR(p, unsigned short, sendSize);
		memcpy(p, sendMsg.c_str(), sendMsg.size());

		if(!m_viewsock->sendData(sendBuff, sendSize)) {
			delete [] sendBuff;			
			return NULL; 
		}

		ViewImageInfo info(IMAGE_DATA_WINDOWS_BMP, COLORBIT_24, IMAGE_320X240);
		ViewImage *view = new ViewImage(info);

		int headerSize = sizeof(unsigned short) + sizeof(double)*2;
		int imageSize = 320*240*3;

		// �o�b�t�@
		char *headerBuff = new char[headerSize];

		if(!m_viewsock->recvData(headerBuff, headerSize)) {
			delete [] headerBuff;
			return NULL; 
		}

		// �w�b�_
		p = headerBuff;
		double fov;
		double ar;
		unsigned short result = BINARY_GET_DATA_S_INCR(p, unsigned short);
		// �G���e�B�e�B��������Ȃ�����
		if(result == 2){
			char tmp[64];
			sprintf_s(tmp, 64,"captureView: cannot find entity [%s]", entityName.c_str());
			MessageBox( NULL, tmp, "Error", MB_OK);
			delete [] headerBuff;
			return NULL;
		}
		// �J������������Ȃ�����
		else if (result == 3){
			char tmp[64];
			sprintf_s(tmp, 64,"captureView: %s doesn't have camera id %d", entityName.c_str(), camID);
			MessageBox( NULL, tmp, "Error", MB_OK);
			delete [] headerBuff;
			return NULL;
		}
		// ���܂�������
		else{
			fov = BINARY_GET_DOUBLE_INCR(p);
			ar = BINARY_GET_DOUBLE_INCR(p);
			view->setFOVy(fov);
			view->setAspectRatio(ar);
		}

		delete [] headerBuff;

		char *imageBuff = new char[imageSize];
		if(!m_viewsock->recvData(imageBuff, imageSize)) {
			delete [] imageBuff;
			return NULL; 
		}

		// delete�̓��[�U�[�ɔC����
		view->setBuffer(imageBuff);
		return view;
	}

	unsigned char SIGService::distanceSensor(std::string entityName, double offset, double range, int camID, ColorBitType ctype)
	{
		if(!m_connectedView){
			MessageBox( NULL, "distanceSensor: Service is not connected to viewer", "Error", MB_OK);
			return 255;
		}

		std::string cameraID = IntToString(camID);
		std::string sendMsg = entityName + "," + cameraID;

		int tmpsize = sizeof(double) * 2 + sizeof(unsigned short)*2;
		int sendSize = sendMsg.size() + tmpsize;

		char *sendBuff = new char[sendSize];
		char *p = sendBuff;

		BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0002);
		BINARY_SET_DATA_S_INCR(p, unsigned short, sendSize);
		BINARY_SET_DOUBLE_INCR(p, offset);
		BINARY_SET_DOUBLE_INCR(p, range);
		memcpy(p, sendMsg.c_str(), sendMsg.size());

		if(!m_viewsock->sendData(sendBuff, sendSize)) {
			delete [] sendBuff;			
			return 255; 
		}

		delete [] sendBuff;
		int recvSize = sizeof(unsigned short) + 1;

		char *recvBuff = new char[recvSize];
		if(!m_viewsock->recvData(recvBuff, recvSize)) {
			delete [] recvBuff;
			return 255; 
		}
		p = recvBuff;

		// ���ʎ擾
		unsigned short result = BINARY_GET_DATA_S_INCR(p, unsigned short);
		// �G���e�B�e�B��������Ȃ�����
		if(result == 2){
			char tmp[128];
			sprintf_s(tmp, 128,"distanceSensor: cannot find entity [%s]", entityName.c_str());
			MessageBox( NULL, tmp, "Error", MB_OK);
			delete [] recvBuff;
			return 255;
		}
		// �J������������Ȃ�����
		else if (result == 3){
			char tmp[128];
			sprintf_s(tmp, 128,"distanceSensor: %s doesn't have camera [id: %d]", entityName.c_str(), camID);
			MessageBox( NULL, tmp, "Error", MB_OK);
			delete [] recvBuff;
			return 255;
		}
		unsigned char distance = p[0];
		delete [] recvBuff;
		return distance;
	}

	ViewImage* SIGService::distanceSensor1D(std::string entityName, double offset, double range, int camID, ColorBitType ctype, ImageDataSize size)
	{
		if(!m_connectedView){
			MessageBox( NULL, "distanceSensor1D: Service is not connected to viewer", "Error", MB_OK);
			return NULL;
		}
		return getDistanceImage(entityName, offset, range, camID, 1, ctype, size);
	}

	ViewImage* SIGService::distanceSensor2D(std::string entityName, double offset, double range, int camID, ColorBitType ctype, ImageDataSize size)
	{
		if(!m_connectedView){
			MessageBox( NULL, "distanceSensor2D: Service is not connected to viewer", "Error", MB_OK);
			return NULL;
		}
		return getDistanceImage(entityName, offset, range, camID, 2, ctype, size);
	}

	ViewImage* SIGService::getDistanceImage(std::string entityName, double offset, double range, int camID, int dimension, ColorBitType ctype, ImageDataSize size)
	{
		// �擾�f�[�^�̎����J�����h�c,�T�C�Y,�G���e�B�e�B��
		std::string dim = IntToString(dimension);
		std::string cameraID = IntToString(camID);
		std::string camSize = IntToString(size);
		std::string sendMsg = entityName + "," + dim + cameraID + camSize;

		// �w�b�_
		int tmpsize = sizeof(double) * 2 + sizeof(unsigned short)*2;

		// ���M�f�[�^�T�C�Y
		int sendSize = sendMsg.size() + tmpsize;
		char *sendBuff = new char[sendSize];
		char *p = sendBuff;

		BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0004);
		BINARY_SET_DATA_S_INCR(p, unsigned short, sendSize);
		BINARY_SET_DOUBLE_INCR(p, offset);
		BINARY_SET_DOUBLE_INCR(p, range);
		memcpy(p, sendMsg.c_str(), sendMsg.size());

		if(!m_viewsock->sendData(sendBuff, sendSize)){
			delete [] sendBuff;
			return NULL;
		}

		delete [] sendBuff;

		// ���ʂ����󂯎��
		int headerSize = sizeof(unsigned short) + sizeof(double)*2;
		char *header = new char[headerSize];
		if(!m_viewsock->recvData(header, headerSize)){
			return NULL;
		}
		p = header;
		unsigned short result = BINARY_GET_DATA_S_INCR(p, unsigned short);

		// �G���e�B�e�B��������Ȃ�����
		if(result == 2){
			char tmp[128];
			if(dimension == 1)
				sprintf_s(tmp, 128,"distanceSensor1D: cannot find entity [%s]", entityName.c_str());
			else
				sprintf_s(tmp, 128,"distanceSensor2D: cannot find entity [%s]", entityName.c_str());
			MessageBox( NULL, tmp, "Error", MB_OK);
			delete [] header;
			return NULL;
		}
		// �J������������Ȃ�����
		else if (result == 3){
			char tmp[128];
			if(dimension == 1)
				sprintf_s(tmp, 128,"distanceSensor1D: %s doesn't have camera [id: %d]", entityName.c_str(), camID);
			else
				sprintf_s(tmp, 128,"distanceSensor2D: %s doesn't have camera [id: %d]", entityName.c_str(), camID);
			MessageBox( NULL, tmp, "Error", MB_OK);
			delete [] header;
			return NULL;
		}

		// ����p�ƃA�X�y�N�g��擾
		double fov = BINARY_GET_DOUBLE_INCR(p);
		double ar = BINARY_GET_DOUBLE_INCR(p);

		delete [] header;

		// ��M�f�[�^�T�C�Y�ݒ�
		int recvSize;
		if(dimension == 1){
			recvSize = 320;
		}
		else if(dimension == 2){
			recvSize = 320*240;
		}
		char *recvBuff = new char[recvSize];
		if(!m_viewsock->recvData(recvBuff, recvSize)) {
			delete [] recvBuff;
			return NULL; 
		}

		ViewImage *img;
		if(dimension == 1){
			ViewImageInfo info(IMAGE_DATA_WINDOWS_BMP, DEPTHBIT_8, IMAGE_320X1);
			img = new ViewImage(info);
		}

		else if(dimension == 2){
			ViewImageInfo info(IMAGE_DATA_WINDOWS_BMP, DEPTHBIT_8, IMAGE_320X240);
			img = new ViewImage(info);
		}
		img->setFOVy(fov);
		img->setAspectRatio(ar);
		img->setBuffer(recvBuff);
		//unsigned char *recvData = new unsigne
		//ViewImage *img;
		return img;
	}

	ViewImage* SIGService::getDepthImage(std::string entityName, double offset, double range, int camID, ColorBitType ctype, ImageDataSize size)
	{
		if(!m_connectedView){
			MessageBox( NULL, "getDepthImage: Service is not connected to viewer", "Error", MB_OK);
			return NULL;
		}

		// �擾�f�[�^�̎����J�����h�c,�T�C�Y,�G���e�B�e�B��
		std::string cameraID = IntToString(camID);
		std::string camSize = IntToString(size);
		std::string sendMsg = entityName + "," + cameraID + camSize;

		// �w�b�_
		int tmpsize = sizeof(double) * 2 + sizeof(unsigned short)*2;

		// ���M�f�[�^�T�C�Y
		int sendSize = sendMsg.size() + tmpsize;
		char *sendBuff = new char[sendSize];
		char *p = sendBuff;

		BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0005);
		BINARY_SET_DATA_S_INCR(p, unsigned short, sendSize);
		BINARY_SET_DOUBLE_INCR(p, offset);
		BINARY_SET_DOUBLE_INCR(p, range);
		memcpy(p, sendMsg.c_str(), sendMsg.size());

		if(!m_viewsock->sendData(sendBuff, sendSize)){
			delete [] sendBuff;
			return NULL;
		}

		delete [] sendBuff;

		// ���ʂ����󂯎��
		int headerSize = sizeof(unsigned short) + sizeof(double)*2;
		char *header = new char[headerSize];
		if(!m_viewsock->recvData(header, headerSize)){
			return NULL;
		}
		p = header;
		unsigned short result = BINARY_GET_DATA_S(p, unsigned short);

		// �G���e�B�e�B��������Ȃ�����
		if(result == 2){
			char tmp[128];
			sprintf_s(tmp, 128,"getDepthImage: cannot find entity [%s]", entityName.c_str());
			MessageBox( NULL, tmp, "Error", MB_OK);
			delete [] header;
			return NULL;
		}
		// �J������������Ȃ�����
		else if (result == 3){
			char tmp[128];
			sprintf_s(tmp, 128,"getDepthImage: %s doesn't have camera [id: %d]", entityName.c_str(), camID);
			MessageBox( NULL, tmp, "Error", MB_OK);
			delete [] header;
			return NULL;
		}

		// ����p�ƃA�X�y�N�g��擾
		double fov = BINARY_GET_DOUBLE_INCR(p);
		double ar = BINARY_GET_DOUBLE_INCR(p);

		delete [] header;

		// ��M�f�[�^�T�C�Y�ݒ�
		int recvSize = 320*240;

		char *recvBuff = new char[recvSize];
		if(!m_viewsock->recvData(recvBuff, recvSize)) {
			delete [] recvBuff;
			return NULL; 
		}

		ViewImage *img;
		ViewImageInfo info(IMAGE_DATA_WINDOWS_BMP, DEPTHBIT_8, IMAGE_320X240);
		img = new ViewImage(info);

		img->setFOVy(fov);
		img->setAspectRatio(ar);
		img->setBuffer(recvBuff);

		return img;
		return NULL;
	}

	bool SIGService::setSunLight(double value, double theta)
	{
		if(!m_connectedView){
			MessageBox( NULL, "setSunLight: Service is not connected to viewer", "Error", MB_OK);
			return NULL;
		}
		int tmpsize = sizeof(double) * 2 + sizeof(unsigned short) * 2;
		int sendSize = tmpsize;

		char *sendBuff = new char[sendSize];
		char *p = sendBuff;

		BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0006);
		BINARY_SET_DATA_S_INCR(p, unsigned short, sendSize);
		BINARY_SET_DOUBLE_INCR(p, value);
		BINARY_SET_DOUBLE_INCR(p, theta);
		
		if(!m_viewsock->sendData(sendBuff, sendSize)) {
			delete [] sendBuff;			
			return NULL; 
		}
		return true;
	}

	bool SIGService::setShadow()
	{
		double value=0;
		if(!m_connectedView){
			MessageBox( NULL, "setShadow: Service is not connected to viewer", "Error", MB_OK);
			return NULL;
		}
		int tmpsize = sizeof(double) + sizeof(unsigned short) * 2;
		int sendSize = tmpsize;

		char *sendBuff = new char[sendSize];
		char *p = sendBuff;

		BINARY_SET_DATA_S_INCR(p, unsigned short, 0x0007);
		BINARY_SET_DATA_S_INCR(p, unsigned short, sendSize);
		BINARY_SET_DOUBLE_INCR(p, value);

		if(!m_viewsock->sendData(sendBuff, sendSize)) {
			delete [] sendBuff;			
			return NULL; 
		}
		return true;
	}
};