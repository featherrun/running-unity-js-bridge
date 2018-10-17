/**
 * Running.JS Lib.
 * Copyright (c) 2017-2018, featherrun
 * All rights reserved.
 */

using System;
using System.Text;
using System.Runtime.InteropServices;

namespace Running.JS
{
    public class JSApi
    {

#if UNITY_IPHONE
        const string DLL = "__Internal";
#else
        const string DLL = "sm";
#endif


        [StructLayout(LayoutKind.Sequential)]
        public struct Words
        {
            public IntPtr s;
            public uint length;
            public override string ToString()
            {
                return PtrToString(s, length);
            }
        }


        // Delegate
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool OnImportScripts(string s);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public delegate bool OnAlert(IntPtr s, uint length);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool OnInvoke(int obj, int func, int argc);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool OnError(string filename, uint lineno, string message);


        // Function
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern bool startupEngine(OnImportScripts onImportScripts, OnAlert onAlert, OnInvoke onInvoke, OnError onError);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern bool shutdownEngine();

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern bool evaluate(byte[] script, byte[] filename);
        public static bool evaluateString(string script, string filename)
        {
            return evaluate(Encoding.UTF8.GetBytes(script), Encoding.ASCII.GetBytes(filename));
        }


        // Get Invoke Args
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern bool readBoolean(int i);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int readInt(int i);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern double readDouble(int i);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern Words readString(int i);
        public static string readUTF(int i)
        {
            return readString(i).ToString();
        }


        // Set Invoke Rval
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void returnBoolean(bool b);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void returnInt(int i);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void returnIntArray(int[] iArr, uint length);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void returnDouble(double d);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void returnDoubleArray(double[] dArr, uint length);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void returnString([MarshalAs(UnmanagedType.LPWStr)] string s);


        // Call JS
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern bool callByName(byte[] func);
        public static bool callByString(string func)
        {
            return callByName(Encoding.ASCII.GetBytes(func));
        }

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern bool load(int obj, int func);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern bool loadBoolean(int obj, int func, bool b);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern bool loadInt(int obj, int func, int i);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern bool loadDouble(int obj, int func, double d);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern bool loadString(int obj, int func, [MarshalAs(UnmanagedType.LPWStr)] string s);


        /// <summary>
        /// 指针=>字符串
        /// </summary>
        /// <param name="s"></param>
        /// <param name="length"></param>
        /// <returns></returns>
        public static string PtrToString(IntPtr s, uint length)
        {
            if (s == IntPtr.Zero || length == 0 || length > int.MaxValue) {
                return null;
            }
            byte[] buff = new byte[length];
            Marshal.Copy(s, buff, 0, buff.Length);
            return Encoding.UTF8.GetString(buff);
        }
    }
}
