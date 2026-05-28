using WinFormsApp2.Logic;
using System;
using System.Windows.Forms;

namespace WinFormsApp2
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (!int.TryParse(textBox1.Text, out int a) || !int.TryParse(textBox2.Text, out int x))
            {
                label5.Text = "Ошибка: неверный формат чисел!";
                return;
            }

            var result = Calculator.Calculate(a, x);

            if (result.errorMessage != null)
            {
                label5.Text = result.errorMessage;
                return;
            }

            label5.Text = "";
            label3.Text = $"Результат F в десятичной системе: {result.F}";
            label4.Text = $"Целая часть в двоичной системе: {result.binaryString}";
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {

        }

        private void label3_Click(object sender, EventArgs e)
        {

        }
    }
}
