#include <windows.h>
#include <stdio.h>

// Ranges: 65...90 - 97...122 - 48...57 - 43 47
static char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
         // Perform any necessary cleanup.
            break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}


typedef struct {
    unsigned int d:6;
    unsigned int c:6;
    unsigned int b:6;
    unsigned int a:6;
} TRIPLET;

#define bswap(x) (*x = ((unsigned char*)x)[2] | (((unsigned char*)x)[1]<<8) | (((unsigned char*)x)[0]<<16) )

unsigned char *decoder_table = 0;


__declspec(dllexport)
unsigned int dec2(char* s, unsigned int inlen, char** out, int params)
{
    char* p;
    unsigned int i=0, n, j;

    if (!*out)
        *out = (char*) malloc(((inlen/4)+3)*3);
    p = *out;

    if (!decoder_table)
    {
        int z = 0;
        decoder_table = malloc(256);
        memset(decoder_table, 64, 256);
        while (z < 64)
            decoder_table[(unsigned int) alphabet[z]] = (unsigned char) z++;
    }
    
    while (*s)
    {
        if (*s == '=' || *s == '\r' || *s == '\n') {
            s++;
            continue;
        }

        n = decoder_table[*s];
        if (n == 64) {
            strcpy(*out, "BAD");
            return 4;
        }
    
        switch (i) {
            case 0:
                j = n << 2;
                break;
            case 1:
                j |= ((n & 0x30) >> 4);
                *p++ = j;
                j = (n & 0xF) << 4;
                break;
            case 2:
                j |= ((n & 0x3C) >> 2);
                *p++ = j;
                j = (n & 0x3) << 6;
                break;
            case 3:
                j |= n;
                *p++ = j;
                break;
        }
        s++;
        i = (i+1)%4;
    }
    *p = 0;
    return p - *out;
}


__declspec(dllexport)
char* enc3(char* s, unsigned int len, int params)
{
    unsigned int cb = 0;
    char *result, *p;
    
    cb = (len/3+1)*4+2;
    if (params&1)
        cb += (cb/76+1)*2;
    if (params&2)
        cb += (cb/64+1)*2;
    result = (char*) malloc(cb);
    p = result;
    cb=0;

    //~ X        CCDDDDDD BBBBCCCC AAAAAABB
    //~ 00000000 00100001 00100001 01101111
    while (len >= 3) {
        unsigned int x = *((unsigned int*)s); // LITTLE ENDIAN MATH ONLY!
        *p++ = alphabet[x<<24>>26];
        *p++ = alphabet[x<<30>>26 | x<<16>>28];
        *p++ = alphabet[x<<20>>28<<2 | x<<8>>30];
        *p++ = alphabet[x<<10>>26];
        len-=3;
        s+=3;
        cb+=4;
        if ( (params&1 && cb==76) || (params&2 && cb==64) )
        {
            cb=0;
            *p++ = '\r';
            *p++ = '\n';
        }
    }

    if (len)
    {
        unsigned int x = *((unsigned int*)s);
        if (len == 2) {
            *p++ = alphabet[x<<24>>26];
            *p++ = alphabet[x<<30>>26 | x<<16>>28];
            *p++ = alphabet[x<<20>>28<<2];
            *p++ = '=';
        }
        if (len == 1) {
            *p++ = alphabet[x<<24>>26];
            *p++ = alphabet[x<<30>>26];
            *p++ = '=';
            *p++ = '=';
        }
    }
    *p = 0;
    return result;
}


#if 0
__declspec(dllexport)
char* enc4(char* s, unsigned int len, int params)
{
    unsigned int cb = 0;
    char *result, *p;
    
    cb = (len/3+1)*4+2;
    if (params&1)
        cb += (cb/76+1)*2;
    if (params&2)
        cb += (cb/64+1)*2;
    result = (char*) malloc(cb);
    p = result;
    cb=0;

    //~ X        CCDDDDDD BBBBCCCC AAAAAABB
    //~ 00000000 00100001 00100001 01101111
    while (len >= 3) {
        unsigned int x = *((unsigned int*)s); // LITTLE ENDIAN MATH ONLY!
        *((int*)p) =        alphabet[x<<10>>26] << 24 |
                                    alphabet[x<<20>>28<<2 | x<<8>>30] << 16 |
                                    alphabet[x<<30>>26 | x<<16>>28] << 8 |
                                    alphabet[x<<24>>26];
        p+=4;
        s+=3;
        len-=3;
        cb+=4;
        if ( (params&1 && cb==76) || (params&2 && cb==64) )
        {
            cb=0;
            *p++ = '\r';
            *p++ = '\n';
        }
    }

    if (len)
    {
        unsigned int x = *((unsigned int*)s);
        if (len == 2) {
            *((int*)p) = '=' << 24 | alphabet[x<<20>>28<<2] << 16 | alphabet[x<<30>>26 | x<<16>>28] << 8 | alphabet[x<<24>>26];
            p+=4;
        }
        if (len == 1) {
            *((int*)p) = '=' << 24 | '=' << 16 | alphabet[x<<30>>26] << 8 | alphabet[x<<24>>26];
            p+=4;
        }
    }
    *p = 0;
    return result;
}


__declspec(dllexport)
char* enc2(char* s, unsigned int len, int params)
{
    unsigned int cb = 0;
    char *result, *p;
    unsigned int x;
    TRIPLET* t = (TRIPLET*) &x;
    
    cb = (len/3+1)*4+2;
    if (params&1)
        cb += (cb/76+1)*2;
    if (params&2)
        cb += (cb/64+1)*2;
    result = (char*) malloc(cb);
    p = result;
    cb=0;

    while (len >= 3) {
        x = *((int*)s);
        bswap(&x);
        *p++ = alphabet[t->a];
        *p++ = alphabet[t->b];
        *p++ = alphabet[t->c];
        *p++ = alphabet[t->d];
        len-=3;
        s+=3;
        cb+=4;
        if ( (params&1 && cb==76) || (params&2 && cb==64) )
        {
            cb=0;
            *p++ = '\r';
            *p++ = '\n';
        }
    }

    if (len)
    {
        x = *((int*)s);
        bswap(&x);
        *p++ = alphabet[t->a];
        *p++ = alphabet[t->b];
        if (len==2)
            *p++ = alphabet[t->c];
        *p++ = '=';
        if (len==1)
            *p++ = '=';
    }
    *p = 0;
    return result;
}


__declspec(dllexport)
char* enc1(char* s, unsigned int len, int params)
{
    unsigned int i = 0;
    char *result = (char*) malloc((len/3+1)*4);
    char *p = result;
    unsigned short word;
    char todo=0;
    while (len--)
    {
        word = (word<<8) | *s++;
        todo+=8;
        while (todo >= 6)
        {
            todo-=6;
            *p++ = alphabet[(word>>todo) & 0x3F];
        }
    }

    if (todo == 2)
    {
        *p++ = alphabet[(word&3)<<4];
        *p++ = '=';
        *p++ = '=';
    }
    else if (todo == 4)
    {
        *p++ = alphabet[(word&0xF)<<2];
        *p++ = '=';
    }
    *p = 0;
    return result;
}


__declspec(dllexport)
char* enc(char* s, unsigned int len, int params)
{
    unsigned int i = 0;
    char *result = (char*) malloc(len*2);
    char *p = result;
    //~ MessageBox(0, s, "Base64 encoded command line", MB_OK);

    while (len--)
    {
        switch (i) {
            case 0:
                    *p++ = alphabet[ (*s & 0xFC) >> 2 ];
                    *p = *s & 3;
                    break;
            case 1:
                    *p = alphabet[ *p << 4 | ((*s & 0xF0) >> 4)];
                    *++p = *s & 0xF;
                    break;
            case 2:
                    *p = alphabet[*p << 2 | ((*s & 0xC0) >> 6) ];
                    *++p = alphabet[*s & 0x3F];
                    p++;
                    break;
        }
        i = ++i % 3;
        s++;
    }

    switch (i)
    {
        case 1:
            *p = alphabet[*p << 4];
            *++p = '=';
            *++p = '=';
            p++;
            break;
        case 2:
            *p = alphabet[*p << 2];
            *++p = '=';
            p++;
            break;
    }

    *p = 0;

    return result;
}
#endif


LRESULT
CALLBACK
MainWndProc1(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
   char cmd[512];

   switch(msg)
   {
	  case WM_COMMAND:
		  if (LOWORD(wParam) == 0x100 && GetDlgItemText(hWnd,0x100,cmd,512))
		  {
			SetDlgItemText(hWnd,0x101,enc3(cmd,lstrlen(cmd),0));
		  }
		  break;

      case WM_DESTROY:
         PostQuitMessage(0);
         break;

      default:
         return(DefWindowProc(hWnd,msg,wParam,lParam));
   }

   return 0;
}


LRESULT
CALLBACK
MainWndProc2(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    char cmd[512];

   switch(msg)
   {
	  case WM_COMMAND:
		  if (LOWORD(wParam) == 0x100 && GetDlgItemText(hWnd,0x100,cmd,512))
		  {
            char* s=0;
            dec2(cmd, lstrlen(cmd), &s, 0);
			SetDlgItemText(hWnd,0x101,s);
		  }
		  break;

      case WM_DESTROY:
         PostQuitMessage(0);
         break;

      default:
         return(DefWindowProc(hWnd,msg,wParam,lParam));
   }

   return 0;
}


// Esempio di funzione richiamabile da RunDll32 (cerca EncW->EncA->Enc)
__declspec(dllexport)
void EncA(HWND hwnd, HINSTANCE hInstance, LPSTR lpszCmdLine, int nCmdShow)
{
   WNDCLASSEX wc;
   MSG msg;
   HWND hmyWnd;

   wc.cbSize = sizeof(WNDCLASSEX);
   wc.lpszClassName = "B64CLASS";
   wc.lpfnWndProc = MainWndProc1;
   wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
   wc.hInstance = hInstance;
   wc.hIcon = 0;
   wc.hIconSm = 0;
   wc.hCursor = 0;
   wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
   wc.lpszMenuName = 0;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;

   RegisterClassEx(&wc);

   hmyWnd = CreateWindowEx(WS_EX_STATICEDGE,wc.lpszClassName,"Base64 Encoder",WS_OVERLAPPEDWINDOW,80,40,320,100,0,0,hInstance,0);

   CreateWindowEx(WS_EX_STATICEDGE,"EDIT",0,WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,2,2,308,20,hmyWnd,(HMENU)0x100,hInstance,0);
   CreateWindowEx(WS_EX_STATICEDGE,"EDIT",0,WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_READONLY,2,38,308,20,hmyWnd,(HMENU)0x101,hInstance,0);
   SetDlgItemText(hmyWnd,0x100,lpszCmdLine);

   ShowWindow(hmyWnd,SW_SHOWNORMAL);

   while(GetMessage(&msg,0,0,0) != 0) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
}

__declspec(dllexport)
void DecA(HWND hwnd, HINSTANCE hInstance, LPSTR lpszCmdLine, int nCmdShow)
{
   WNDCLASSEX wc;
   MSG msg;
   HWND hmyWnd;
   //~ HFONT hf;

   wc.cbSize = sizeof(WNDCLASSEX);
   wc.lpszClassName = "B64CLASS";
   wc.lpfnWndProc = MainWndProc2;
   wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
   wc.hInstance = hInstance;
   wc.hIcon = 0;
   wc.hIconSm = 0;
   wc.hCursor = 0;
   wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
   wc.lpszMenuName = 0;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;

   RegisterClassEx(&wc);

   hmyWnd = CreateWindowEx(WS_EX_STATICEDGE,wc.lpszClassName,"Base64 Decoder",WS_OVERLAPPEDWINDOW,80,40,320,100,0,0,hInstance,0);

   CreateWindowEx(WS_EX_STATICEDGE,"EDIT",0,WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,2,2,308,20,hmyWnd,(HMENU)0x100,hInstance,0);
   CreateWindowEx(WS_EX_STATICEDGE,"EDIT",0,WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_READONLY,2,38,308,20,hmyWnd,(HMENU)0x101,hInstance,0);
   SetDlgItemText(hmyWnd,0x100,lpszCmdLine);
   //~ hf = CreateFont(12, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, "Verdana");
   //~ SendMessage(hmyWnd,WM_SETFONT,(WPARAM)hf,0);
   ShowWindow(hmyWnd,SW_SHOWNORMAL);

   while(GetMessage(&msg,0,0,0) != 0) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
}

#ifdef MAIN
void main(int argc, char** argv)
{
    char* s;
    dec2(argv[1], strlen(argv[1]), &s, 0);
    puts(s);
}
#endif
