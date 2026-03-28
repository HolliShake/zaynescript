
using System;
using System.Threading.Tasks;

class Program
{
    static async Task<string> TopLevel()
    {
        Console.WriteLine("From top level");
        return "Hello";
    }

    static async Task<string> AsyncFn()
    {
        await TopLevel();
        Console.WriteLine("Called!!");
        return "Resolve me!";
    }

    static async Task<int> CallMe()
    {
        Console.WriteLine(await AsyncFn());
        Console.WriteLine(await AsyncFn());
        Console.WriteLine(await AsyncFn());
        return 1;
    }

    public static void Main(string[] args)
    {
        Console.WriteLine(CallMe());
        Console.WriteLine("AUTO");
        Console.WriteLine(CallMe());
    }
}