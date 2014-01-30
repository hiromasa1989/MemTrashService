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

// SIGServiceを継承したクラスを作成します  
class MyService : public sigverse::SIGService  
{  
  
public:  
    MyService(std::string name) : SIGService(name){};  
    ~MyService();  
    // メッセージ受信時に呼び出される関数  
    void onRecvMsg(sigverse::RecvMsgEvent &evt);  

	// カメラの画像を記憶する
	void MemImage(std::string name);

	void MemRef(std::string name);
	void MemInp(std::string name);
	void ExtArea(std::string name);
	void SetLight();

    // 定期的に呼び出される関数  
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
    // 切断します  
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
    // メッセージ送信元とメッセージを取得します  
    std::string sender = evt.getSender();  
    char *all_msg = (char*)evt.getMsg();  

	char *delim = " ";
	char *ctx;

	char *header = strtok_s(all_msg,delim, &ctx);
  
    // コントローラからの物体記憶リクエスト
    if(strcmp(header, "MemObject") == 0) {  
		this->MemImage(sender);
		// 記憶完了をコントローラに送る
		char send_text[256];
		sprintf_s(send_text, 256, "MemOK");
		this->sendMsgToCtr(sender, send_text);
	}
	if(strcmp(header, "MemRef") == 0) {  
		this->MemRef(sender);
		// 記憶完了をコントローラに送る
		char send_text[256];
		sprintf_s(send_text, 256, "MemOK");
		this->sendMsgToCtr(sender, send_text);
	}
	if(strcmp(header, "MemInp") == 0) {  
		this->MemInp(sender);
		// 記憶完了をコントローラに送る
		char send_text[256];
		sprintf_s(send_text, 256, "MemOK");
		this->sendMsgToCtr(sender, send_text);
	}
	if(strcmp(header, "ExtArea") == 0) {  
		this->ExtArea(sender);
		// 記憶完了をコントローラに送る
		char send_text[256];
		sprintf_s(send_text, 256, "MemOK");
		this->sendMsgToCtr(sender, send_text);
	}
	if(strcmp(header, "SetLight") == 0) {  
		this->SetLight();
		// 記憶完了をコントローラに送る
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
	// カメラ画像を取得する
	image = this->captureView(sender);
	
	// 画像データをバッファにコピー
	char* buf_image = image->getBuffer();

	IplImage* dstImage = cvCreateImage(cvSize(area_w, area_h), IPL_DEPTH_8U, 3);
	//画像データポインタを取得する
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
	//画像データの解放
    cvReleaseImage(&dstImage);
}

//物体切出し用背景画像取得
void MyService::MemRef(std::string sender)  
{
	// カメラ画像を取得する
	image = this->captureView(sender);
	
	//BMPで保存
	char fname[256];
	sprintf_s(fname, 256, "Scene/reference.bmp");
	image->saveAsWindowsBMP(fname);

	//画像配列に保存
	char* src;
	char* dst;
	reference = new char[image->getBufferLength()];
	src = image->getBuffer();
	dst = reference;
	for(int i=0; i<image->getBufferLength(); i++){
		*dst++ = *src++;
	}
}

//物体切出し用入力画像取得
void MyService::MemInp(std::string sender)  
{
	// カメラ画像を取得する
	image = this->captureView(sender);
	
	//BMPで保存
	char fname[256];
	sprintf_s(fname, 256, "Scene/input.bmp");
	image->saveAsWindowsBMP(fname);

	//画像配列に保存
	char* src;
	char* dst;
	input = new char[image->getBufferLength()];
	src = image->getBuffer();
	dst = input;
	for(int i=0; i<image->getBufferLength(); i++){
		*dst++ = *src++;
	}
}

//物体切出し領域計算
void MyService::ExtArea(std::string sender)  
{
	char* buf_sub = new char[WIDTH*HEIGHT];

	// Rの値が変わらないのでそれで差分を計算
	char *pCur = reference;
    char *pPrv = input;
    for (int y = 0; y < HEIGHT; y++){
        for (int x = 0; x < WIDTH; x++){
            buf_sub[x + y * WIDTH] = abs(*pCur - *pPrv);
			pCur+=3;
			pPrv+=3;
		}
	}

	//物体領域を座標で記憶
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
	// クラスMyServiceのインスタンスを作成します  
    MyService srv("MemObj");
	srv.onInit();

    // サーバに接続します  
    srv.connect(HOST_NAME, PORT_NUM);  
	srv.connectToViewer();
	
    // メインループをスタートさせます  
    srv.startLoop();  


    return 0;  
}  