#include <Windows.h>

/*
 * ------------------------------------------
 * **/

LRESULT CALLBACK // Aquí es donde se procesan todos los mensajes que se envían a la ventana 
MainWindowCallback(HWND Window, // manejador de ventana
    UINT Message, // Mensaje (WM_*)
    WPARAM WParam,
    LPARAM LParam)
{
    LRESULT Result = 0; // Si una aplicación procesa este mensaje retorna cero

    switch (Message)
    {
    case WM_SIZE:
    {
        OutputDebugStringA("WM_SIZE\n");
    }break;

    case WM_DESTROY:
    {
        OutputDebugStringA("WM_DESTROY\n");
    }break;

    case WM_CLOSE:
    {
        OutputDebugStringA("WM_CLOSE\n");
    }break;

    case WM_ACTIVATEAPP:
    {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
    }break;

    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint( // Prepara la ventana para pintar y rellena la estructura PAINTSTRUCT
            Window, // Manejador de ventana
            &Paint); // estructura PAINTSTRUCT

        int X = Paint.rcPaint.left;
        int Y = Paint.rcPaint.top;
        LONG Heigh = Paint.rcPaint.bottom - Paint.rcPaint.top; // estructura RECT en estructura PAINTSTRUCT
        LONG Width = Paint.rcPaint.right - Paint.rcPaint.left;
        static DWORD Operation = WHITENESS; // color asociado con el índice 1 en la paleta física. (blanco)
                                            // pinta el rectángulo especificado usando el brush seleccionado en el contexto de dispositivo
        PatBlt(DeviceContext, X, Y, Width, Heigh, Operation);
        if (Operation == WHITENESS)
        {
            Operation = BLACKNESS;
        }
        else
        {
            Operation = WHITENESS;
        }

        EndPaint(Window, &Paint); // Fin de pintar ventana

    }break;

    default:
    {
        // OutputDebugStringA("default\n");
        Result = DefWindowProc(Window, Message, WParam, LParam);
    }break;
    }

    return(Result);
}

int CALLBACK
WinMain(HINSTANCE Instance,
    HINSTANCE hPrevInstance,
    LPSTR CommandLine,
    int ShowCode)
{
    WNDCLASS WindowClass = {};

    // TODO(casey): Probar si HREDRAW/VREDRAW/OWNDC todavía importan
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; // Estilos de clase (CS_*) que no debe confundirse con estilos de ventanas (WS_*) 
    WindowClass.lpfnWndProc = MainWindowCallback; // Puntero al procedimiento de ventana para esta clase de ventana.
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon;

    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass)) // registrar la clase de ventana, si va bien retorna una clase atom que la identifica
    {
        HWND WindowHandle =
            CreateWindowEx( // Si va bien retorna un manejador a la ventana
                0, // estilo extendido de la ventana
                WindowClass.lpszClassName,
                "Handmade Hero", // Nombre de la ventana
                WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Estilo de la ventana (WS_*)
                CW_USEDEFAULT, // Posición horizontal
                CW_USEDEFAULT, // Posición vertical
                CW_USEDEFAULT, // Ancho
                CW_USEDEFAULT, // Alto
                0, // Manejador a parent
                0, // Menú
                Instance,
                0);
        if (WindowHandle)
        {

            for (;;)
            {
                MSG Message;
                BOOL MessageResult = GetMessage(&Message, // Mensaje
                    0, // Manejador a ventana que se quieren manejar los mensajes
                    0, // wMsgFilterMin
                    0); // wMsgFilterMax
                if (MessageResult > 0)
                {
                    TranslateMessage(&Message); // traduce mensajes con teclas virtuales, en mensajes con carácteres
                    DispatchMessage(&Message);
                }
                else
                {
                    break;
                }
            }
        }
    }
    else
    {
        // TODO(casey): Logging
    }
    return(0);
}