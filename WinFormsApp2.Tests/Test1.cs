using System.Windows.Forms;
using System.Reflection;

namespace WinFormsApp2.Tests;

[TestClass]
public sealed class Test1
{
    [TestMethod]
    public void TestMethod1()
    {
    }

    [TestMethod]
    public void Test_X_Is_Zero_ShowsError()
    {
        // Arrange
        var form = new Form1();

        var textBox1Field = typeof(Form1).GetField("textBox1", BindingFlags.NonPublic | BindingFlags.Instance);
        var textBox1 = (TextBox)textBox1Field.GetValue(form);
        textBox1.Text = "1";

        var textBox2Field = typeof(Form1).GetField("textBox2", BindingFlags.NonPublic | BindingFlags.Instance);
        var textBox2 = (TextBox)textBox2Field.GetValue(form);
        textBox2.Text = "0";

        var button1Field = typeof(Form1).GetField("button1", BindingFlags.NonPublic | BindingFlags.Instance);
        var button1 = (Button)button1Field.GetValue(form);

        var label5Field = typeof(Form1).GetField("label5", BindingFlags.NonPublic | BindingFlags.Instance);
        var label5 = (Label)label5Field.GetValue(form);

        var clickMethod = typeof(Form1).GetMethod("button1_Click", BindingFlags.NonPublic | BindingFlags.Instance);

        // Act
        clickMethod.Invoke(form, new object[] { button1, EventArgs.Empty });

        // Assert
        Assert.AreEqual("Ошибка: x не может быть нулём!", label5.Text);
    }
}
