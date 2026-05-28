using System;
using Xunit;
using WinFormsApp2;

namespace WinFormsApp2.Tests
{
    public class LogicTests
    {
        [Fact]
        public void Calculate_InvalidInputA_ThrowsFormatException()
        {
            Assert.Throws<FormatException>(() =>
                Logic.Calculate("abc", "123", out double _, out string _));
        }

        [Fact]
        public void Calculate_InvalidInputX_ThrowsFormatException()
        {
            Assert.Throws<FormatException>(() =>
                Logic.Calculate("123", "abc", out double _, out string _));
        }

        [Fact]
        public void Calculate_XIsZero_ReturnsErrorMessage()
        {
            string result = Logic.Calculate("1", "0", out double fValue, out string binaryString);
            Assert.Equal("Ошибка: x не может быть нулём!", result);
        }

        [Fact]
        public void Calculate_MathDomainError_CubeLessSquare_ReturnsErrorMessage()
        {
            string result = Logic.Calculate("2", "4", out double fValue, out string binaryString);
            Assert.Equal("Ошибка: a³ должно быть >= x²!", result);
        }

        [Fact]
        public void Calculate_MathDomainError_AbsALessAbsX_ReturnsErrorMessage()
        {
            string result = Logic.Calculate("5", "3", out double fValue, out string binaryString);
            Assert.Equal("Ошибка: |a| должно быть <= |x|!", result);
        }

        [Fact]
        public void Calculate_ValidInput_ReturnsOK()
        {
            string result = Logic.Calculate("3", "3", out double fValue, out string binaryString);
            Assert.Equal("OK", result);
        }
    }
}
