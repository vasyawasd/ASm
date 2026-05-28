using System;

namespace WinFormsApp2.Logic
{
    public class Calculator
    {
        public static (string? errorMessage, double F, string? binaryString) Calculate(int a, int x)
        {
            if (x == 0)
            {
                return ("Ошибка: x не может быть нулём!", 0, null);
            }

            if (a * a * a - x * x < 0)
            {
                return ("Ошибка: a³ должно быть >= x²!", 0, null);
            }

            if (Math.Abs(a) > Math.Abs(x))
            {
                return ("Ошибка: |a| должно быть <= |x|!", 0, null);
            }

            double part1 = Math.Sqrt(a * a * a - x * x) / x;
            double part2 = (a * a / 2.0) * Math.Acos((double)Math.Abs(a) / x) * Math.Tan(x * x);
            double F = part1 + part2;

            int integerPart = (int)Math.Floor(F);
            string binaryString = Convert.ToString(integerPart, 2);

            return (null, F, binaryString);
        }
    }
}
