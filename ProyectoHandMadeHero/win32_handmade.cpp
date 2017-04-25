#include <Windows.h>

#define internal static
#define local_persist static 
#define global_variable static 

//TODO(casey): Esto es global por ahora
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo; // Estructura que define las dimensiones y color de un DIB
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

// Un mapa de bits independiente del dispositivo (DIB) contiene una
// tabla de colores de píxeles de color RGB
internal void
Win32ResizeDIBSection(int Width, int Height)
{
    //TODO(casey): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.

    if (BitmapHandle)
    {
        DeleteObject(BitmapHandle);
    }

    if (!BitmapDeviceContext)
    {
        // crea un contexto de dispositivo de memoria (DC) compatible con el dispositivo especificado
        // Si el manejador es NULL, crea un DC de memoria compatible con la pantalla actual de la aplicación
        // TODO(casey): Should we recreate these under certain special circunstances
        BitmapDeviceContext = CreateCompatibleDC(0);
    }


    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader); // Número de bytes requeridos por la estructura
    BitmapInfo.bmiHeader.biWidth = Width; // ancho del bitmap
    BitmapInfo.bmiHeader.biHeight = Height; // alto
    BitmapInfo.bmiHeader.biPlanes = 1; // número de planos del dispositivo de destino
    BitmapInfo.bmiHeader.biBitCount = 32; // número de bits por pixel
    BitmapInfo.bmiHeader.biCompression = BI_RGB;// Tipo de comprensión del bitmap (jpeg,png,rle,rgb no comprimido)

                                                //TODO(casey): Based on ssylvan's suggestion, maybe we can just
                                                // allocate this ourselves?
    BitmapHandle = CreateDIBSection( // crea un DIB que se puede escribir en forma directa.
        BitmapDeviceContext, // manejador de DeviceContext
        &BitmapInfo, // Puntero a estructura BITMAPINFO
        DIB_RGB_COLORS, // Tipo de datos en el miembro bmiColors de BITMAPINFO
        &BitmapMemory, // Un puntero a una variable con un puntero a la ubicación de los bits de DIB.
        0, // Manejador a objeto que la función usará para crear el DIB
        0); // Offset desde el inicio

            //ReleaseDC(0, BitmapDeviceContext); // libera un contexto de dispositivo (DC), para su uso por otras aplicaciones
}

internal void
Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
    // copia los datos de color de un rectángulo de píxeles de una imagen de DIB,
    // JPEG o PNG en el rectángulo de destino especificado
    StretchDIBits(DeviceContext,
        X, Y, Width, Height, // Medidas rectángulo de destino
        X, Y, Width, Height, // Medidas rectángulo de origen
        BitmapMemory, // Puntero a imagen en un arreglo de bytes
        &BitmapInfo, // Puntero a una estructura BITMAPINFO con información de DIB
        DIB_RGB_COLORS, // Paleta de colores
        SRCCOPY); // Raster Operation, indica como serán combinados los pixels (Copia en este caso)
}

LRESULT CALLBACK // Aquí es donde se procesan todos los mensajes que se envían a la ventana 
Win32MainWindowCallback(HWND Window, // manejador de ventana
    UINT Message, // Mensaje (WM_*)
    WPARAM WParam,
    LPARAM LParam)
{
    LRESULT Result = 0; // Si una aplicación procesa este mensaje retorna cero

    switch (Message)
    {
    case WM_SIZE:
    {
        RECT ClientRect;
        GetClientRect(Window, &ClientRect);// Recupera las coordenadas de un área de cliente de ventana
        int Width = ClientRect.right - ClientRect.left;
        int Heigh = ClientRect.bottom - ClientRect.top;
        Win32ResizeDIBSection(Width, Heigh);

    }break;

    case WM_CLOSE:
    {
        //TODO(casey): ¿Manejar esto con un mensaje al usuario?
        Running = false;
        OutputDebugStringA("WM_CLOSE\n");
    }break;

    case WM_ACTIVATEAPP:
    {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
    }break;

    case WM_DESTROY:
    {
        //TODO(casey): ¿Manejar esto como un error - recrear ventana?
        Running = false;
        OutputDebugStringA("WM_DESTROY\n");
    }break;

    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint( // Prepara la ventana para pintar y rellena la estructura PAINTSTRUCT
            Window, // Manejador de ventana
            &Paint); // estructura PAINTSTRUCT

        int X = Paint.rcPaint.left;
        int Y = Paint.rcPaint.top;
        int Width = Paint.rcPaint.right - Paint.rcPaint.left;
        int Height = Paint.rcPaint.bottom - Paint.rcPaint.top; // estructura RECT en estructura PAINTSTRUCT
        Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
        /*local_persist DWORD Operation = WHITENESS; // color asociado con el índice 1 en la paleta física. (blanco)
        //pinta el rectángulo especificado usando el brush seleccionado en el contexto de dispositivo
        PatBlt(DeviceContext, X, Y, Width, Height, Operation);
        if (Operation == WHITENESS)
        {
        Operation = BLACKNESS;
        }
        else
        {
        Operation = WHITENESS;
        }*/

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
    WindowClass.lpfnWndProc = Win32MainWindowCallback; // Puntero al procedimiento de ventana para esta clase de ventana.
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
            Running = true;
            while (Running)
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
                    break; // Salir si MessageResult es cero o menor
                }
            }
        }
    }
    else
    {
        // TODO(casey): Logging
    }
    return(0); // Salir
}