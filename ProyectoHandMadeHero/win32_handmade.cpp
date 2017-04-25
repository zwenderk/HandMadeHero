#include <Windows.h>
#include <stdint.h>

#define internal static
#define local_persist static 
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

//TODO(casey): Esto es global por ahora
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo; // Estructura que define las dimensiones y color de un DIB
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void
RenderWeirGradient(int XOffset, int YOffset)
{
    int Width = BitmapWidth;
    int Height = BitmapHeight;

    int Pitch = Width*BytesPerPixel;
    uint8 *Row = (uint8 *)BitmapMemory;
    for (int Y = 0;
    Y < BitmapHeight;
        ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0;
        X < BitmapWidth;
            ++X)
        {
            /*
            Pixel 32 bits

            Pixel in memory: BB GG RR xx
            Register:        xx RR GG BB


            LITTLE ENDIAN ARCHITECTURE 0x xxBBGGRR
            */

            uint8 Blue = (X + XOffset);
            uint8 Green = (Y + YOffset);


            *Pixel++ = ((Green << 8) | Blue);

        }
        Row += Pitch;
    }
}

// Un mapa de bits independiente del dispositivo (DIB) contiene una
// tabla de colores de píxeles de color RGB
internal void
Win32ResizeDIBSection(int Width, int Height)
{
    //TODO(casey): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.

    if (BitmapMemory)
    {
        VirtualFree( // Libera memória
            BitmapMemory, // puntero a la memoria a liberar 
            0, // tamaño en bytes
            MEM_RELEASE); // Tipo de operación
    }

    //if (BitmapHandle)
    //{
    //    DeleteObject(BitmapHandle);
    //}

    //if(!BitmapDeviceContext)
    //{
    // crea un contexto de dispositivo de memoria (DC) compatible con el dispositivo especificado
    // Si el manejador es NULL, crea un DC de memoria compatible con la pantalla actual de la aplicación
    // TODO(casey): Should we recreate these under certain special circunstances
    //   BitmapDeviceContext = CreateCompatibleDC(0); 
    //}

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader); // Número de bytes requeridos por la estructura
    BitmapInfo.bmiHeader.biWidth = BitmapWidth; // ancho del bitmap
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight; // alto
    BitmapInfo.bmiHeader.biPlanes = 1; // número de planos del dispositivo de destino
    BitmapInfo.bmiHeader.biBitCount = 32; // número de bits por pixel
    BitmapInfo.bmiHeader.biCompression = BI_RGB;// Tipo de comprensión del bitmap (jpeg,png,rle,rgb no comprimido)

                                                //NOTE(casey): Thank you to Cris Hecker of Spy Party fame
                                                // for clarifyng the deal with StretchDIBits and BitBlt!
                                                // No more DC for us.

    int BitmapMemorySize = (BitmapWidth*BitmapHeight)*BytesPerPixel;

    BitmapMemory = VirtualAlloc(// Reserva con ceros una región de páginas de memoria.
        0, // Dirección de inicio 0 -> lo determina el sistema
        BitmapMemorySize, // Tamaño de la región en bytes, si es 0 se redondea a la siguiente página
        MEM_COMMIT, // Tipo de memoria
        PAGE_READWRITE); // Protección de la memoria

    RenderWeirGradient(0, 0);
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width, int Height)
{
    /*

    */
    int WindowWidth = ClientRect->right - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;

    // copia los datos de color de un rectángulo de píxeles de una imagen de DIB,
    // JPEG o PNG en el rectángulo de destino especificado
    StretchDIBits(DeviceContext,
        //X, Y, Width, Height, // Medidas rectángulo de destino
        //X, Y, Width, Height, // Medidas rectángulo de origen
        0, 0, BitmapWidth, BitmapHeight, // Medidas rectángulo de destino
        0, 0, WindowWidth, WindowHeight, // Medidas rectángulo de origen
        BitmapMemory, // Puntero a imagen en un arreglo de bytes
        &BitmapInfo, // Puntero a una estructura BITMAPINFO con información de DIB
        DIB_RGB_COLORS, // Paleta de colores
        SRCCOPY); // Raster Operation, indica como serán combinados los pixels (Copia en este caso)
}

LRESULT CALLBACK
Win32MainWindowCallback( // Aquí es donde se procesan todos los mensajes que se envían a la ventana 
    HWND Window, // manejador de ventana
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

        RECT ClientRect;
        GetClientRect(Window, &ClientRect);// Recupera las coordenadas de un área de cliente de ventana
        Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
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
        HWND Window =
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
        if (Window)
        {
            int XOffset = 0;
            int YOffset = 0;
            Running = true;
            while (Running)
            {

                MSG Message;
                while (PeekMessage( // Despacha mensajes entrantes, y recupera el mensaje (si existe).
                    &Message, // Un puntero a estructura MSG
                    0, 0, 0,
                    PM_REMOVE)) // Como manejar el mensaje (mensaje se borra despues de procesarlo)
                {
                    if (Message.message == WM_QUIT)
                    {
                        Running = false;
                    }

                    TranslateMessage(&Message); // traduce mensajes con teclas virtuales, en mensajes con carácteres
                    DispatchMessage(&Message);
                }
                RenderWeirGradient(XOffset, YOffset); // Colorea pixels directamente

                                                      // Recupera un identificador para un contexto de dispositivo (DC) para una ventana o para toda la pantalla
                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);// Recupera las coordenadas de un área de cliente de ventana
                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;

                Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
                ReleaseDC(Window, DeviceContext);

                ++XOffset;
            }
        }
        else
        {
            //TODO:(casey): Logging
        }
    }
    else
    {
        // TODO(casey): Logging
    }
    return(0); // Salir
}