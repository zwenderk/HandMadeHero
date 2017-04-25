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

struct win32_offscreen_buffer
{
    BITMAPINFO Info; // Estructura que define las dimensiones y color de un DIB
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    //int BytesPerPixel;
};

//TODO(casey): Esto es global por ahora
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;

struct win32_window_dimension
{
    int Width;
    int Height;
};

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);// Recupera las coordenadas de un área de cliente de ventana
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}




internal void
RenderWeirGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
    // TODO(casey): Let's see what the optimizer does

    uint8 *Row = (uint8 *)Buffer.Memory;
    for (int Y = 0;
    Y < Buffer.Height;
        ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0;
        X < Buffer.Width;
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
        Row += Buffer.Pitch; // Buffer.Pitch son los bytes horizontales de la imagen
    }
}

// Un mapa de bits independiente del dispositivo (DIB) contiene una
// tabla de colores de píxeles de color RGB
internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    //TODO(casey): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.

    if (Buffer->Memory)
    {
        VirtualFree( // Libera memória
            Buffer->Memory, // puntero a la memoria a liberar 
            0, // tamaño en bytes
            MEM_RELEASE); // Tipo de operación
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    int BytesPerPixel = 4;

    // NOTE(casey): When the biHeight field is negative, this is the clue to
    // Windows to treat this bitmap as top-down, not bottom-up, meaning that
    // the first three bytes of the image are the color for the top left pixel
    // in the bitmap, not the bottom left!

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader); // Número de bytes requeridos por la estructura
    Buffer->Info.bmiHeader.biWidth = Buffer->Width; // ancho del bitmap
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // alto
    Buffer->Info.bmiHeader.biPlanes = 1; // número de planos del dispositivo de destino
    Buffer->Info.bmiHeader.biBitCount = 32; // número de bits por pixel
    Buffer->Info.bmiHeader.biCompression = BI_RGB;// Tipo de comprensión del bitmap (jpeg,png,rle,rgb no comprimido)

                                                  //NOTE(casey): Thank you to Cris Hecker of Spy Party fame
                                                  // for clarifyng the deal with StretchDIBits and BitBlt!
                                                  // No more DC for us.

    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;

    Buffer->Memory = VirtualAlloc(// Reserva con ceros una región de páginas de memoria.
        0, // Dirección de inicio 0 -> lo determina el sistema
        BitmapMemorySize, // Tamaño de la región en bytes, si es 0 se redondea a la siguiente página
        MEM_COMMIT, // Tipo de memoria
        PAGE_READWRITE); // Protección de la memoria

    Buffer->Pitch = Width*BytesPerPixel;
    //TODO(casey): Probably clear this to black


}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext,
    int WindowWidth, int WindowHeight,
    win32_offscreen_buffer Buffer,
    int X, int Y, int Width, int Height)
{
    //TODO(casey): Aspect ratio correction
    //TODO(casey): Play with stretch modes
    // copia los datos de color de un rectángulo de píxeles de una imagen de DIB,
    // JPEG o PNG en el rectángulo de destino especificado
    StretchDIBits(DeviceContext, // DC que recibirá los datos
        0, 0, WindowWidth, WindowHeight, // Medidas rectángulo de destino
        0, 0, Buffer.Width, Buffer.Height, // Medidas rectángulo de origen
        Buffer.Memory, // Puntero a imagen en un arreglo de bytes
        &Buffer.Info, // Puntero a una estructura BITMAPINFO con información de DIB
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
        //win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        //Win32ResizeDIBSection(&GlobalBackbuffer, Dimension.Width, Dimension.Height); // Aqui el sistema rellena GlobalBackbuffer

    }break;

    case WM_CLOSE:
    {
        //TODO(casey): ¿Manejar esto con un mensaje al usuario?
        GlobalRunning = false;
        OutputDebugStringA("WM_CLOSE\n");
    }break;

    case WM_ACTIVATEAPP:
    {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
    }break;

    case WM_DESTROY:
    {
        //TODO(casey): ¿Manejar esto como un error - recrear ventana?
        GlobalRunning = false;
        OutputDebugStringA("WM_DESTROY\n");
    }break;

    case WM_PAINT:
    {
        OutputDebugStringA("WM_PAINT\n");
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint( // Prepara la ventana para pintar y rellena la estructura PAINTSTRUCT
            Window, // Manejador de ventana
            &Paint); // estructura PAINTSTRUCT que se devuelve rellena

        int X = Paint.rcPaint.left;
        int Y = Paint.rcPaint.top;
        int Width = Paint.rcPaint.right - Paint.rcPaint.left;
        int Height = Paint.rcPaint.bottom - Paint.rcPaint.top; // estructura RECT en estructura PAINTSTRUCT

        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
            GlobalBackbuffer, X, Y, Width, Height);


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
    //win32_window_dimension Dimension = Win32GetWindowDimension(Window);
    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720); // Aqui el sistema rellena GlobalBackbuffer

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // Estilos de clase (CS_*) que no debe confundirse con estilos de ventanas (WS_*) 
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

            // NOTE(casey): Since we specified CS_OWNDC, we can just
            // get one device context and use it forever because we
            // are not sharing it with anyone.
            // Recupera un identificador para un contexto de dispositivo (DC) para una ventana o para toda la pantalla
            HDC DeviceContext = GetDC(Window);

            int XOffset = 0;
            int YOffset = 0;


            GlobalRunning = true;
            while (GlobalRunning)
            {
                MSG Message;

                while (PeekMessage( // Despacha mensajes entrantes, y recupera el mensaje (si existe).
                    &Message, // Un puntero a estructura MSG
                    0, 0, 0,
                    PM_REMOVE)) // Como manejar el mensaje (mensaje se borra despues de procesarlo)
                {
                    if (Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }

                    TranslateMessage(&Message); // traduce mensajes con teclas virtuales, en mensajes con carácteres
                    DispatchMessage(&Message);
                }
                RenderWeirGradient(GlobalBackbuffer, XOffset, YOffset); // Colorea pixels directamente



                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
                    GlobalBackbuffer,
                    0, 0, Dimension.Width, Dimension.Height);


                ++XOffset;
                YOffset += 2;
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