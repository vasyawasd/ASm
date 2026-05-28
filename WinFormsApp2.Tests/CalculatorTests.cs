using System;
using Xunit;
using WinFormsApp2.Logic;

namespace WinFormsApp2.Tests
{
    public class CalculatorTests
    {
        [Fact]
        public void Calculate_XIsZero_ReturnsError()
        {
            // Arrange
            int a = 1;
            int x = 0;

            // Act
            var result = Calculator.Calculate(a, x);

            // Assert
            Assert.Equal("Ошибка: x не может быть нулём!", result.errorMessage);
        }

        [Fact]
        public void Calculate_ACubedLessThanXSquared_ReturnsError()
        {
            // Arrange
            int a = 1; // a^3 = 1
            int x = 2; // x^2 = 4 (1 < 4) -> Error

            // Act
            var result = Calculator.Calculate(a, x);

            // Assert
            Assert.Equal("Ошибка: a³ должно быть >= x²!", result.errorMessage);
        }

        [Fact]
        public void Calculate_AbsAGreaterThanAbsX_ReturnsError()
        {
            // Arrange
            int a = 5; // a^3 = 125
            int x = 4; // x^2 = 16

            // Act
            var result = Calculator.Calculate(a, x);

            // Assert
            Assert.Equal("Ошибка: |a| должно быть <= |x|!", result.errorMessage);
        }

        [Fact]
        public void Calculate_ValidInput_ReturnsCorrectFAndBinaryString()
        {
            // Arrange
            int a = 2; // a^3 = 8
            int x = 2; // x^2 = 4, |2| <= |2|

            // Act
            var result = Calculator.Calculate(a, x);

            // Assert
            Assert.Null(result.errorMessage);

            // F calculation:
            // part1 = sqrt(8 - 4) / 2 = sqrt(4) / 2 = 2 / 2 = 1
            // part2 = (4 / 2.0) * acos(2 / 2) * tan(4)
            //       = 2 * acos(1) * tan(4)
            //       = 2 * 0 * tan(4) = 0
            // F = 1 + 0 = 1
            Assert.Equal(1.0, result.F, 5);

            // integerPart = floor(1.0) = 1
            // binary = 1
            Assert.Equal("1", result.binaryString);
        }
    }
}
