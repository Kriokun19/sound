
#include <windows.h>
#include <windowsx.h>
#define ClassName "ServerWindow"
#define AppName "Server"

#define PORT 1001

HWND hWnd, button, button2;
HANDLE ServerHandle;
DWORD id = 0u;
int xView = 0, yView = 0, k;
DWORD ServerOn(HWND);
const int F = 50, Fmax = 10000, N = 1000, Ns = 200;

//typedef int Data[N];
static int BufData[8][N];

//Оконная процедура
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

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

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	button = CreateWindow(
		"button", "Начать прием", WS_VISIBLE | WS_CHILD, 550, 40, 150, 30, hWnd, NULL, NULL, NULL
		);
	button2 = CreateWindow(
		"button", "Остановить прием", WS_VISIBLE | WS_CHILD, 550, 100, 150, 30, hWnd, NULL, NULL, NULL
		);
	EnableWindow(button2, 0);

	//Цикл обработки сообщений
	while (GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (msg.wParam);
}

DWORD ServerOn(HWND hWnd){
	//Инициализация WinSock
	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws)){
		//
	}

	//Инициализация сокета
	SOCKET Server;
	Server = socket(AF_INET, SOCK_DGRAM, 0);

	sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = INADDR_ANY; //изменить тут?
	local_addr.sin_port = htons(PORT);
	bind(Server, (sockaddr*)&local_addr, sizeof(local_addr));

	//sockaddr_in client_addr;
	int bsize;
	const int BufferCount = 8;

	HWAVEOUT waveOut;

	int bufsize;
	WAVEFORMATEX Format;
	Format.nChannels = 1;
	Format.wFormatTag = WAVE_FORMAT_PCM;
	Format.nSamplesPerSec = 8000;
	Format.wBitsPerSample = 8;
	Format.nBlockAlign = 1;
	Format.cbSize = 0;
	Format.nAvgBytesPerSec = 8000;
	bufsize = (Format.nAvgBytesPerSec * 2) / 16;
	waveOutOpen(&waveOut, WAVE_MAPPER, &Format, NULL, 0, CALLBACK_NULL);

	WAVEHDR fHeaders[BufferCount];
	for (int i = 0; i < BufferCount; ++i){
		fHeaders[i].dwFlags = WHDR_INQUEUE;
		fHeaders[i].dwBufferLength = bufsize;
		fHeaders[i].dwBytesRecorded = 0;
		fHeaders[i].dwUser = 0;
		fHeaders[i].dwLoops = 1;
		fHeaders[i].lpData = PCHAR(GlobalAlloc(GMEM_FIXED, bufsize));
	}

	int i = 0;
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while (1){
		// принимаем только сообщение WM_QUIT для завершение потока
		if (PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE))
			break; // если приняли прерываем цикл
		if (fHeaders[i].dwUser == 0){
			bsize = recv(Server, fHeaders[i].lpData, fHeaders[i].dwBufferLength, 0);
			waveOutPrepareHeader(waveOut, &fHeaders[i], sizeof(WAVEHDR));
			waveOutWrite(waveOut, &fHeaders[i], sizeof(WAVEHDR));
			for (int j = 0; j < N; ++j){
				BufData[i][j] = fHeaders[i].lpData[j];
			}
			SendMessage(hWnd, WM_PAINT, i, 0);
			fHeaders[i].dwUser = 0;
		}
		if (i == BufferCount - 1){
			for (int i = 0; i < BufferCount; ++i){
				fHeaders[i].dwFlags = WHDR_INQUEUE;
				fHeaders[i].dwBufferLength = bufsize;
				fHeaders[i].dwBytesRecorded = 0;
				fHeaders[i].dwUser = 0;
				fHeaders[i].dwLoops = 1;
			}
			i = 0;
		}
		++i;
	}
	waveOutReset(waveOut);
	for (int i = 0; i < BufferCount; ++i)
		waveOutUnprepareHeader(waveOut, &fHeaders[i], sizeof(WAVEHDR));
	waveOutClose(waveOut);
	for (int i = 0; i < BufferCount; ++i)
		GlobalFree(fHeaders[i].lpData);
	closesocket(Server);
	
	return 0u;
}

//Оконная процедура
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc, Thdc; //создаём контекст устройства
	PAINTSTRUCT ps; //создаём экземпляр структуры графического вывода
	HPEN hPen; //создаём перо 

	switch (msg)
	{
	case WM_SIZE:
		xView = LOWORD(lParam);
		yView = HIWORD(lParam);
		break;
	case WM_COMMAND:
		if ((HWND)lParam == button){
			//Начать запись
			EnableWindow(button, 0);
			EnableWindow(button2, 1);
			//Запускаем сервер в другом потоке 
			//чтобы можно было продолжать работу с окном
			ServerHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerOn, hWnd, 0, &id);
			WaitForSingleObject(ServerHandle, 500);
		}
		if ((HWND)lParam == button2){
			EnableWindow(button2, 0);
			EnableWindow(button, 1);
			PostThreadMessage(id, WM_QUIT, 0, 0);
		}
		break;
	case WM_PAINT:
		InvalidateRect(hWnd, NULL, 1); //Обновляем экран
		hdc = BeginPaint(hWnd, &ps);
		SetMapMode(hdc, MM_ISOTROPIC); //логические единицы отображаем, как физические
		SetWindowExtEx(hdc, 500, 500, NULL); //Длина осей
		SetViewportExtEx(hdc, xView, -yView, NULL); //Определяем облась вывода
		SetViewportOrgEx(hdc, 0, yView / 2, NULL); //Начало координат

		//Цвет пера
		hPen = CreatePen(1, 1, RGB(255, 255, 255));
		SelectObject(hdc, hPen);

		k = wParam;
		MoveToEx(hdc, 0, 0, NULL);
		//Отрисуем звук
		for (int i = 0; i < N; ++i){
			LineTo(hdc, i, BufData[k][i]/3.0);
		}

		EndPaint(hWnd, &ps);
		ReleaseDC(hWnd, hdc);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}