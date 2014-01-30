//#include "stdafx.h"  
#include "SIGService.h"   
#include "MyCV.h"
#include <tchar.h>  
#include <string>  

#define _USE_MATH_DEFINES
#include <math.h>

// for server
#define HOST_NAME	"192.168.122.137"
#define PORT_NUM	9000

#define WIDTH 320
#define HEIGHT 240
#define SCENE 24


struct vector3{
	double x;
	double y;
	double z;
};

// SIGService���p�������N���X���쐬���܂�  
class MyService : public sigverse::SIGService  
{  
  
public:  
    MyService(std::string name) : SIGService(name){};  
    ~MyService();  
    // ���b�Z�[�W��M���ɌĂяo�����֐�  
    void onRecvMsg(sigverse::RecvMsgEvent &evt);  

	// �J�����̉摜���L������
	void MemImage(std::string name);

	void MemRef(std::string name);
	void MemInp(std::string name);
	void ExtArea(std::string name);
	void SetLight();

    // ����I�ɌĂяo�����֐�  
    double onAction();


	void onInit();

private:
	int mem_num;
	sigverse::ViewImage *image;
	char* reference;
	char* input;
	char* sub;

	int area_xs, area_ys;
	int area_xe, area_ye;
	int area_w, area_h;
	double theta;//Direction of the sun from original point
	double value;//Brightness value of the sun
	double div;//Division number for theta ( half day is pi)
	double inc;//Incriment direction for theta

};

MyService::~MyService()  
{  
    // �ؒf���܂�  
    this->disconnect();  
}  
  
void MyService::onInit()
{
	mem_num = 0;
	div = SCENE;
	inc = M_PI/div;

	theta = 0;
	value = 3.0;

}

double MyService::onAction()  
{  
    return 1.0;  
}  
  
void MyService::onRecvMsg(sigverse::RecvMsgEvent &evt)  
{  
    // ���b�Z�[�W���M���ƃ��b�Z�[�W���擾���܂�  
    std::string sender = evt.getSender();  
    char *all_msg = (char*)evt.getMsg();  

	char *delim = " ";
	char *ctx;

	char *header = strtok_s(all_msg,delim, &ctx);
  
    // �R���g���[������̕��̋L�����N�G�X�g
    if(strcmp(header, "MemObject") == 0) {  
		this->MemImage(sender);
		// �L���������R���g���[���ɑ���
		char send_text[256];
		sprintf_s(send_text, 256, "MemOK");
		this->sendMsgToCtr(sender, send_text);
	}
	if(strcmp(header, "MemRef") == 0) {  
		this->MemRef(sender);
		// �L���������R���g���[���ɑ���
		char send_text[256];
		sprintf_s(send_text, 256, "MemOK");
		this->sendMsgToCtr(sender, send_text);
	}
	if(strcmp(header, "MemInp") == 0) {  
		this->MemInp(sender);
		// �L���������R���g���[���ɑ���
		char send_text[256];
		sprintf_s(send_text, 256, "MemOK");
		this->sendMsgToCtr(sender, send_text);
	}
	if(strcmp(header, "ExtArea") == 0) {  
		this->ExtArea(sender);
		// �L���������R���g���[���ɑ���
		char send_text[256];
		sprintf_s(send_text, 256, "MemOK");
		this->sendMsgToCtr(sender, send_text);
	}
	if(strcmp(header, "SetLight") == 0) {  
		this->SetLight();
		// �L���������R���g���[���ɑ���
		char send_text[256];
		sprintf_s(send_text, 256, "SetOK");
		this->sendMsgToCtr(sender, send_text);
	}
	if(strcmp(header, "SetShadow") == 0) {  
		this->setShadow();
		char send_text[256];
		sprintf_s(send_text, 256, "SetOK");
		this->sendMsgToCtr(sender, send_text);
	}
}  

void MyService::MemImage(std::string sender)  
{
	// �J�����摜���擾����
	image = this->captureView(sender);
	
	// �摜�f�[�^���o�b�t�@�ɃR�s�[
	char* buf_image = image->getBuffer();

	IplImage* dstImage = cvCreateImage(cvSize(area_w, area_h), IPL_DEPTH_8U, 3);
	//�摜�f�[�^�|�C���^���擾����
    uchar *pSrc = (uchar*)buf_image;
    uchar *pDst = (uchar*)dstImage->imageData;
	int x,y;
	y = 0;
	for (int j = area_ys; j < area_ye; j++){
        x = 0;
		for (int i = area_xs; i < area_xe; i++){
			pDst[x*3 + y * dstImage->widthStep] = pSrc[i*3 + j * WIDTH*3];
			pDst[x*3+1 + y * dstImage->widthStep] = pSrc[i*3+1 + j * WIDTH*3];
			pDst[x*3+2 + y * dstImage->widthStep] = pSrc[i*3+2 + j * WIDTH*3];
			x++;
		}
		y++;
	}
	char fname[256];
	sprintf_s(fname, 256, "Image/view_c%d.bmp",mem_num);
	image->saveAsWindowsBMP(fname);
	sprintf_s(fname, 256, "Object/view%d.bmp",mem_num);
	cvSaveImage (fname, dstImage);
	
	mem_num++;
	//�摜�f�[�^�̉��
    cvReleaseImage(&dstImage);
}

//���̐؏o���p�w�i�摜�擾
void MyService::MemRef(std::string sender)  
{
	// �J�����摜���擾����
	image = this->captureView(sender);
	
	//BMP�ŕۑ�
	char fname[256];
	sprintf_s(fname, 256, "Scene/reference.bmp");
	image->saveAsWindowsBMP(fname);

	//�摜�z��ɕۑ�
	char* src;
	char* dst;
	reference = new char[image->getBufferLength()];
	src = image->getBuffer();
	dst = reference;
	for(int i=0; i<image->getBufferLength(); i++){
		*dst++ = *src++;
	}
}

//���̐؏o���p���͉摜�擾
void MyService::MemInp(std::string sender)  
{
	// �J�����摜���擾����
	image = this->captureView(sender);
	
	//BMP�ŕۑ�
	char fname[256];
	sprintf_s(fname, 256, "Scene/input.bmp");
	image->saveAsWindowsBMP(fname);

	//�摜�z��ɕۑ�
	char* src;
	char* dst;
	input = new char[image->getBufferLength()];
	src = image->getBuffer();
	dst = input;
	for(int i=0; i<image->getBufferLength(); i++){
		*dst++ = *src++;
	}
}

//���̐؏o���̈�v�Z
void MyService::ExtArea(std::string sender)  
{
	char* buf_sub = new char[WIDTH*HEIGHT];

	// R�̒l���ς��Ȃ��̂ł���ō������v�Z
	char *pCur = reference;
    char *pPrv = input;
    for (int y = 0; y < HEIGHT; y++){
        for (int x = 0; x < WIDTH; x++){
            buf_sub[x + y * WIDTH] = abs(*pCur - *pPrv);
			pCur+=3;
			pPrv+=3;
		}
	}

	//���̗̈�����W�ŋL��
	int min_x = WIDTH;
	int min_y = HEIGHT;
	int max_x = 0;
	int max_y = 0;
	for(int y=0; y<HEIGHT; y++){
		for(int x=0; x<WIDTH; x++){
			if(buf_sub[x + y * WIDTH]){
				if(x < min_x)	min_x = x;
				if(y < min_y)	min_y = y;
				if(x > max_x)	max_x = x;
				if(y > max_y)	max_y = y;
			}
		}
	}
	area_xs = min_x;
	area_ys = min_y;
	area_xe = max_x;
	area_ye = max_y;
	area_w = max_x-min_x;
	area_h = max_y-min_y;
	std::cout << min_x << "," << min_y << "," << max_x << "," << max_y << std::endl;
}

void MyService::SetLight()  
{
	bool test;
	test = this->setSunLight(value, theta);
	theta += inc;
	
	if(theta > M_PI)
		theta = 0;
}

int main(int argc, char** argv)  
{  
	// �N���XMyService�̃C���X�^���X���쐬���܂�  
    MyService srv("MemObj");
	srv.onInit();

    // �T�[�o�ɐڑ����܂�  
    srv.connect(HOST_NAME, PORT_NUM);  
	srv.connectToViewer();
	
    // ���C�����[�v���X�^�[�g�����܂�  
    srv.startLoop();  


    return 0;  
}  