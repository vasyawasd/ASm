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

            int x = int.Parse(textBox1.Text);

            if (x == 0)
            {
                label1.Text = "x не может быть нулём!";
            }

            if (a * a * a - x * x < 0)
            {
                Console.WriteLine("Ошибка: a³ должно быть >= x²!");
                return;
            }

            if (Math.Abs(a) > Math.Abs(x))
            {
                Console.WriteLine("Ошибка: |a| должно быть <= |x|!");
                return;
            }

            double part1 = Math.Sqrt(a * a * a - x * x) / x;
            double part2 = (a * a / 2) * Math.Acos(Math.Abs(a) / x) * Math.Tan(x * x);
            double F = part1 + part2;


            Console.WriteLine($"Результат F в десятичной системе: {F}");

            int integerPart = (int)Math.Floor(F);
            string binaryString = Convert.ToString(integerPart, 2);
            label1.Text = ($"Целая часть в двоичной системе: {binaryString}");
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {

        }
    }
}

