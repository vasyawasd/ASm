using System;

namespace WinFormsApp2
{
    public class Logic
    {
        public static string Calculate(string aText, string xText, out double fValue, out string binaryString)
        {
            fValue = 0;
            binaryString = "";

            int a = int.Parse(aText);
            int x = int.Parse(xText);

            if (x == 0)
            {
                return "Ошибка: x не может быть нулём!";
            }

            if (a * a * a - x * x < 0)
            {
                return "Ошибка: a³ должно быть >= x²!";
            }

            if (Math.Abs(a) > Math.Abs(x))
            {
                return "Ошибка: |a| должно быть <= |x|!";
            }

            double part1 = Math.Sqrt(a * a * a - x * x) / x;
            double part2 = (a * a / 2) * Math.Acos(Math.Abs(a) / x) * Math.Tan(x * x);
            double F = part1 + part2;

            fValue = F;

            int integerPart = (int)Math.Floor(F);
            binaryString = Convert.ToString(integerPart, 2);

            return "OK";
        }
    }
}
