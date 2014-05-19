
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <math.h>

#define PORT 1001
#define ClassName "ClientWindow"
#define AppName "Client"

static char SERVERADDR[20] = "0.0.0.0";
double Pi = acos(-1.0);
HWND hWnd;
static int bufsize;
WAVEHDR waveHdr;
SOCKET Client;
HWND button, button2, hEdit;
int xSize, ySize,k;
const int F = 50, Fmax = 10000, N=320, Ns=200;

//Функция, к которой обращается устройство записи
void CALLBACK OnWave(HWAVEIN, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);

DWORD ClientOn(HWND);

void ErrorHandle(HWND hwnd)
{
	ShowWindow(hwnd, SW_HIDE);
	MessageBox(hwnd, "Error", AppName, MB_OK);
	DestroyWindow(hwnd);
}

//Оконная процедура
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

HWAVEIN waveIn;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nCmdShow){
	WNDCLASSEX wndClass;
	
	MSG msg;
	//Регистрация оконного класса
	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInst;
	wndClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = GetStockBrush(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = ClassName;
	wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	RegisterClassEx(&wndClass);

	//Создание окна на основе класса
	hWnd = CreateWindowEx(
		0,  //Дополнит. стиль окна
		ClassName,	//Класс окна
		AppName,	//Текст заголовка
		WS_OVERLAPPEDWINDOW,	//Стиль окна
		50, 50,		//Координаты X и Y
		GetSystemMetrics(SM_CXSCREEN) / 2,
		GetSystemMetrics(SM_CYSCREEN) / 2,//Ширина и высота
		NULL,		//Дескриптор родит. окна
		NULL,		//Дескриптор меню
		hInst,		//Описатель экземпляра
		NULL);		//Доп. данные

	button = CreateWindow(
		"button", "Начать отправку", WS_VISIBLE | WS_CHILD, 500, 75, 180, 30, hWnd, NULL, NULL, NULL
		);
	button2 = CreateWindow(
		"button", "Остановить отправку", WS_VISIBLE | WS_CHILD, 500, 150, 180, 30, hWnd, NULL, NULL, NULL
		);
	EnableWindow(button2, 0);
	hEdit = CreateWindow("EDIT", "Введите IP", WS_CHILD | WS_VISIBLE, 40, 50, 200, 20, hWnd, NULL, hInst, NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//Инициализация WinSock
	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws)){
		ErrorHandle(hWnd);
	}

	//Инициализация сокета
	
	Client = socket(AF_INET, SOCK_DGRAM, 0);

	//задаем структуру
	//связь с сервером
	HOSTENT *hst;
	sockaddr_in dest_addr;

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(PORT);

	//Определение IP-адреса узла
	if (inet_addr(SERVERADDR))
		dest_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
	else
	if (hst = (HOSTENT*)gethostbyname(SERVERADDR))
		dest_addr.sin_addr.s_addr = ((unsigned long**)hst->h_addr_list)[0][0];
	else{
		ErrorHandle(hWnd);
	}

	connect(Client, (sockaddr*)&dest_addr, sizeof(dest_addr));

	//LPWAVEFORMATEX Format;
	WAVEFORMATEX Format;
	/* высокое качество
	Format.nChannels = 1;
	Format.wFormatTag = WAVE_FORMAT_PCM;
	Format.nSamplesPerSec = 22050;
	Format.wBitsPerSample = 16;
	Format.nBlockAlign = 2;
	Format.cbSize = 0;
	Format.nAvgBytesPerSec = 44100;
	bufsize = 2*N;*/
	Format.nChannels = 1;
	Format.wFormatTag = WAVE_FORMAT_PCM;
	Format.nSamplesPerSec = 8000;
	Format.wBitsPerSample = 8;
	Format.nBlockAlign = 1;
	Format.cbSize = 0;
	Format.nAvgBytesPerSec = 8000;
	bufsize = (Format.nAvgBytesPerSec * 2) / 16;	
	
	waveInOpen(&waveIn, WAVE_MAPPER, &Format, (DWORD)OnWave, 0, CALLBACK_FUNCTION);

	waveHdr.lpData = PCHAR(GlobalAlloc(GMEM_FIXED, bufsize));
	waveHdr.dwBufferLength = bufsize;
	waveHdr.dwFlags = 0;

	//waveInClose(waveIn);
	//closesocket(Client);
	//WSACleanup();

	//Цикл обработки сообщений
	while (GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (msg.wParam);

}

//Оконная процедура
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc; //создаём контекст устройства
	PAINTSTRUCT ps; //создаём экземпляр структуры графического вывода
	HPEN hPen; //создаём перо
	RECT rect;
	rect.left = 0;
	rect.right = 100;
	rect.bottom = 50;
	rect.top = 0;
	int k;
	const char mes[]="Соединение не установлено!";
	switch (msg)
	{
	case WM_CREATE:
		break;
	case WM_SIZE:
		xSize = LOWORD(lParam);
		ySize = HIWORD(lParam);
		break;
	case WM_COMMAND:
		if ((HWND)lParam == button){
			//Начать запись
			EnableWindow(button, 0);
			EnableWindow(button2, 1);
			EnableWindow(hEdit, 0);
			waveInPrepareHeader(waveIn, &waveHdr, sizeof(WAVEHDR));
			waveInAddBuffer(waveIn, &waveHdr, sizeof(WAVEHDR));
			waveInStart(waveIn);
		}
		if ((HWND)lParam == button2){
			InvalidateRect(hWnd, &rect, 1);
			EnableWindow(button2, 0);
			EnableWindow(button, 1);
			EnableWindow(hEdit, 1);
			//Остановить запись
			waveInUnprepareHeader(waveIn, &waveHdr, sizeof(WAVEHDR));
			waveInStop(waveIn);
		}
		if ((HWND)lParam == hEdit){
			GetWindowText(hEdit, SERVERADDR, sizeof(SERVERADDR));;
		}
		break;
	case WM_PAINT:
		if (wParam == 1){
			hdc = GetDC(hWnd);
			TextOut(hdc, 0, 0, mes, sizeof(mes));
			ReleaseDC(hWnd, hdc);
		}
		if (wParam == 2)
			InvalidateRect(hWnd, &rect, 1);
		break;
	case WM_DESTROY:
		closesocket(Client);
		//GlobalFree(waveHdr.lpData);
		WSACleanup();
		PostQuitMessage(0);
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

//Коллбэк функция
void CALLBACK OnWave(HWAVEIN waveIn, UINT msg, DWORD_PTR Instance,
	DWORD_PTR Param1, DWORD_PTR Param2){
	int count = 0;
	switch (msg){
	case MM_WIM_DATA:
		/*DData alpha, wi, wr;
		int Frq = -100, k; 
		for (int i = 0; i < N; ++i){
		    k = trunc(N*Frq / 8000.0);             //Определим номер расчетной гармоники
			alpha[i] = 2 * cos(2 * Pi*(k / N));    //Расчет коэфф. Альфа
			wr[i] = cos(2 * Pi*(k / N));       //Поворотный коэфф. реальная часть
			wi[i] = sin(2 * Pi*(k / N));       //Поворотный коэфф. мнимая часть
		    Frq = Frq + F;  
			BufData[i] = waveHdr.lpData[i];
		}
		for(int j = 0; j < N; ++j){
			FData[0] = BufData[0];
			FData[1] = BufData[1] + alpha[j + 1] * FData[0];
				for(int i = 2; i < N; ++i){
					FData[i] = BufData[i] + alpha[j + 2] * FData[i - 1] - FData[i - 2];
				}
				wr[j] = FData[N - 1] * wr[j] - FData[N - 2];
				wi[j] = FData[N - 1] * wi[j];
				wr[j] *= wr[j];
				wi[j] *= wi[j];
				Ampl[j] = (sqrt(wr[j] + wi[j]));
		}*/
		//for (int i = 0; i < N; ++i){
			//BufData[i] = waveHdr.lpData[i];
		//}
		//SendMessage(hWnd, WM_PAINT, 0, 0);

		waveInPrepareHeader(waveIn, &waveHdr, sizeof(WAVEHDR));
		waveInAddBuffer(waveIn, &waveHdr, sizeof(WAVEHDR));
		WSADATA ws;
		if (WSAStartup(MAKEWORD(2, 2), &ws)){
			//
		}
		//Инициализация сокета
		Client = socket(AF_INET, SOCK_DGRAM, 0);

		//задаем структуру
		//связь с сервером
		HOSTENT *hst;
		sockaddr_in dest_addr;

		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(PORT);

		//Определение IP-адреса узла
		if (inet_addr(SERVERADDR))
			dest_addr.sin_addr.s_addr = inet_addr(SERVERADDR);
		else
		if (hst = (HOSTENT*)gethostbyname(SERVERADDR))
			dest_addr.sin_addr.s_addr = ((unsigned long**)hst->h_addr_list)[0][0];
		else{
			//ошибка
		}

		//Соединение с сервером
		k=connect(Client, (sockaddr*)&dest_addr, sizeof(dest_addr));
		if (k != 0){
			count = k;
			SendMessage(hWnd, WM_PAINT, 1, 0);
		}
		else if (count != 0){
			count = 0;
			SendMessage(hWnd, WM_PAINT, 2, 0);
		}
		k=send(Client, waveHdr.lpData, waveHdr.dwBufferLength, 0);
		if (k == -1){
			count = k;
			SendMessage(hWnd, WM_PAINT, 1, 0);
			}
		else
		if (count == -1){
			SendMessage(hWnd, WM_PAINT, 2, 0);
			count = 2;
		}
		closesocket(Client);
	}
}
