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
            int a = int.Parse(textBox1.Text);

            int x = int.Parse(textBox2.Text);

            if (x == 0)
            {
                label5.Text = "Ошибка: x не может быть нулём!";
                return;
            }

            if (a * a * a - x * x < 0)
            {
                label5.Text = "Ошибка: a³ должно быть >= x²!";
            }

            if (Math.Abs(a) > Math.Abs(x))
            {
                label5.Text = "Ошибка: |a| должно быть <= |x|!";
            }

            double part1 = Math.Sqrt(a * a * a - x * x) / x;
            double part2 = (a * a / 2) * Math.Acos(Math.Abs(a) / x) * Math.Tan(x * x);
            double F = part1 + part2;


            label3.Text = ($"Результат F в десятичной системе: {F}");

            int integerPart = (int)Math.Floor(F);
            string binaryString = Convert.ToString(integerPart, 2);
            label4.Text = ($"Целая часть в двоичной системе: {binaryString}");
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {

        }

        private void label3_Click(object sender, EventArgs e)
        {

        }
    }
}

