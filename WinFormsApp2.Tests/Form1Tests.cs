using System;
using System.Reflection;
using System.Windows.Forms;
using Xunit;
using WinFormsApp2;

namespace WinFormsApp2.Tests
{
    public class Form1Tests
    {
        [Fact]
        public void NegativeRadicand_ShowsError_And_StopsProcessing()
        {
            // Arrange
            var form = new Form1();

            // Use reflection to access private fields
            var type = form.GetType();
            var textBox1 = (TextBox)type.GetField("textBox1", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(form);
            var textBox2 = (TextBox)type.GetField("textBox2", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(form);
            var label3 = (Label)type.GetField("label3", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(form);
            var label5 = (Label)type.GetField("label5", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(form);

            // Set input values to trigger negative radicand (a³ - x² < 0)
            // e.g. a = 1, x = 2 => 1 - 4 = -3 < 0
            textBox1.Text = "1";
            textBox2.Text = "2";

            // Capture initial state of label3
            string initialLabel3Text = label3.Text;

            var clickMethod = type.GetMethod("button1_Click", BindingFlags.NonPublic | BindingFlags.Instance);

            // Act
            clickMethod.Invoke(form, new object[] { null, EventArgs.Empty });

            // Assert
            Assert.Equal("Ошибка: a³ должно быть >= x²!", label5.Text);

            // Without early return, it calculates Math.Sqrt(-3), which is NaN,
            // and the text becomes "Результат F в десятичной системе: NaN".
            // We want to ensure it remains unchanged.
            Assert.Equal(initialLabel3Text, label3.Text);
        }
    }
}
