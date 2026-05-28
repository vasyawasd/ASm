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
            string errorMessage = Logic.Calculate(textBox1.Text, textBox2.Text, out double F, out string binaryString);

            if (errorMessage != "OK")
            {
                label5.Text = errorMessage;
            }
            else
            {
                label5.Text = "";
                label3.Text = $"Результат F в десятичной системе: {F}";
                label4.Text = $"Целая часть в двоичной системе: {binaryString}";
            }
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {

        }

        private void label3_Click(object sender, EventArgs e)
        {

        }
    }
}
