using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.IO.Ports;
namespace SerialSendAndReceive
{
    public partial class MainWin : Form
    {
        //这玩意Ports模块自带了
        //private bool isPortOpen = false;//串口是否开启

        //各种列表
        private List<string> PortList;//串口列表
        private List<int> BaudList = new List<int>(new int[] { 50, 75, 100, 110, 150, 300, 600, 1200, 2400, 4800,
            9600, 19200, 38400, 43000, 56000, 57600, 115200, 128000, 230400, 256000,
            460800, 500000, 512000, 600000, 750000, 921600, 1000000, 1500000, 2000000 });//波特率列表
        private List<int> DataBitsList = new List<int>(new int[] { 5, 6, 7, 8 });//数据位长度列表
        private List<Text_Value_Pair<StopBits>> StopBitsList = new List<Text_Value_Pair<StopBits>>(new Text_Value_Pair<StopBits>[] {
            new Text_Value_Pair<StopBits>{TextField="0",ValueField=StopBits.None },
            new Text_Value_Pair<StopBits>{TextField="1",ValueField=StopBits.One },
            new Text_Value_Pair<StopBits>{TextField="1.5",ValueField=StopBits.OnePointFive },
            new Text_Value_Pair<StopBits>{TextField="2",ValueField=StopBits.Two }});//停止位长度列表
        private List<Text_Value_Pair<Parity>> ParityList = new List<Text_Value_Pair<Parity>>(new Text_Value_Pair<Parity>[] {
            new Text_Value_Pair<Parity>{TextField="无",ValueField=Parity.None },
            new Text_Value_Pair<Parity>{TextField="奇校验",ValueField=Parity.Odd },
            new Text_Value_Pair<Parity>{TextField="偶校验",ValueField=Parity.Even },
            new Text_Value_Pair<Parity>{TextField="1",ValueField=Parity.Mark },
            new Text_Value_Pair<Parity>{TextField="0",ValueField=Parity.Space },});//校验位列表

        private SerialPort serial;//串口
        private bool IniFin = false;//是否初始化结束

        private int StopBitSelectIndex = 1;//用来修复某些设置下特定停止位长度无法使用而造成程序崩溃的BUG
        private void InitializeUI()//UI初始化
        {
            //串口ComboBox
            PortList = new List<string>(SerialPort.GetPortNames());
            if (PortList.Count == 0)
            {
                MessageBox.Show("当前设备上未检测到可用串口！");
                Environment.Exit(0);
            }
            PortSelect.DataSource = PortList;
            PortSelect.SelectedIndex = 0;

            //波特率ComboBox
            BaudSelect.DataSource = BaudList;
            BaudSelect.SelectedIndex = 16;

            //数据位ComboBox
            DataBitsSelect.DataSource = DataBitsList;
            DataBitsSelect.SelectedIndex = DataBitsList.Count - 1;

            //停止位ComboBox
            StopBitsSelect.DataSource = StopBitsList;
            StopBitsSelect.DisplayMember = "TextField";
            StopBitsSelect.ValueMember = "ValueField";
            StopBitsSelect.SelectedValue = StopBits.One;

            //校验位ComboBox
            ParitySelect.DataSource = ParityList;
            ParitySelect.DisplayMember = "TextField";
            ParitySelect.ValueMember = "ValueField";
            ParitySelect.SelectedValue = Parity.Even;
        }
        private string TimeStamp => DateTime.Now.ToString("yyyy/MM/dd h:mm:ss.ffff");//时间戳
        public MainWin()//系统自带的
        {
            InitializeComponent();
        }
        private void MainForm_Load(object sender, EventArgs e)//主窗口加载
        {
            //固定窗口大小
            MaximizeBox = false;
            MaximumSize = Size;
            MinimumSize = Size;
            InitializeUI();//初始化UI
            serial = new SerialPort()//串口
            {
                PortName = PortSelect.Text,
                BaudRate = (int)BaudSelect.SelectedValue,
                DataBits = (int)DataBitsSelect.SelectedValue,
                StopBits = (StopBits)StopBitsSelect.SelectedValue,
                Parity = (Parity)ParitySelect.SelectedValue,
            };
            serial.DataReceived += Serial_DataReceived;//串口接收事件
            MessageInput.KeyDown += MessageInput_KeyDown;//输入框回车事件
            IniFin = true;//初始化完成
        }
        //事件
        //自定义事件
        private void Serial_DataReceived(object sender, SerialDataReceivedEventArgs e)//串口数据接收事件
        {
            string data = string.Empty;
            while (serial.BytesToRead > 0) data += serial.ReadExisting();
            if (HexModeCheck.Checked)
            {
                string HEXdata = string.Empty;
                for (int i = 0; i < data.Length; i++) HEXdata += (Convert.ToString((char)data[i], 16) + " ");
                data = HEXdata;
            }
            Invoke((EventHandler)delegate
            {
                DisplayArea.AppendText("[" + TimeStamp + " RECV]" + data + Environment.NewLine);
            });
        }
        private void MessageInput_KeyDown(object sender, KeyEventArgs e)//输入框回车事件
        {
            if (e.KeyCode == Keys.Enter)
            {
                if (serial.IsOpen)
                {
                    if (MessageInput.Text.Length != 0)
                    {
                        serial.Write(MessageInput.Text);
                        DisplayArea.AppendText("[" + TimeStamp + " SEND]" + MessageInput.Text + Environment.NewLine);
                        MessageInput.Clear();
                    }
                }
                else
                {
                    MessageBox.Show("请先打开串口！");
                }
            }
        }
        //各种控件的系统自带事件
        private void PortControlBtn_Click(object sender, EventArgs e)//串口开关
        {
            if (serial.IsOpen)//串口已打开，需关闭
            {
                serial.Close();
                PortControlBtn.Text = "开启";
                Text = "串口通讯器 - 已关闭";
            }
            else//串口已关闭，需打开
            {
                OpenSerial();
            }
        }
        private void OpenSerial()
        {
            try
            {
                serial.Open();
                PortControlBtn.Text = "关闭";
                Text = "串口通讯器 - " + PortSelect.Text;
            }
            catch
            {
                PortControlBtn.Text = "开启";
                DisplayArea.AppendText("串口打开未成功！" + Environment.NewLine);
                Text = "串口通讯器 - 已关闭";
            }
        }
        private void SendBtn_Click(object sender, EventArgs e)//发送按钮
        {
            if (serial.IsOpen)
            {
                if (MessageInput.Text.Length != 0)
                {
                    serial.Write(MessageInput.Text);
                    DisplayArea.AppendText("[" + TimeStamp + " SEND]" + MessageInput.Text + Environment.NewLine);
                    MessageInput.Clear();
                }
            }
            else
            {
                MessageBox.Show("请先打开串口！");
            }
        }
        private void PortSelect_SelectedIndexChanged(object sender, EventArgs e)//串口选择
        {
            if (!IniFin) return;
            if (serial.IsOpen) Text = "串口通讯器 - " + PortSelect.Text;
            bool isPortOnCache = serial.IsOpen;
            serial.Close();
            serial.PortName = PortSelect.Text;
            if (isPortOnCache) OpenSerial();
        }
        private void BaudSelect_SelectedIndexChanged(object sender, EventArgs e)//波特率选择
        {
            if (!IniFin) return;
            bool isPortOnCache = serial.IsOpen;
            serial.Close();
            serial.BaudRate = (int)BaudSelect.SelectedValue;
            if (isPortOnCache) OpenSerial();
        }
        private void DataBitsSelect_SelectedIndexChanged(object sender, EventArgs e)//数据位位数选择
        {
            if (!IniFin) return;
            bool isPortOnCache = serial.IsOpen;
            serial.Close();
            serial.DataBits = (int)DataBitsSelect.SelectedValue;
            if (isPortOnCache) OpenSerial();
        }
        private void StopBitsSelect_SelectedIndexChanged(object sender, EventArgs e)//结束位位数选择
        {
            if (!IniFin) return;
            bool isPortOnCache = serial.IsOpen;
            serial.Close();
            try
            {
                serial.StopBits = (StopBits)StopBitsSelect.SelectedValue;
            }
            catch
            {
                StopBitsSelect.SelectedIndex = StopBitSelectIndex;
                MessageBox.Show("当前状态下该结束位长度不可用！");
            }
            if (isPortOnCache) OpenSerial();
            StopBitSelectIndex = StopBitsSelect.SelectedIndex;
        }
        private void ParitySelect_SelectedIndexChanged(object sender, EventArgs e)//奇偶校验模式选择
        {
            if (!IniFin) return;
            bool isPortOnCache = serial.IsOpen;
            serial.Close();
            serial.Parity = (Parity)ParitySelect.SelectedValue;
            if (isPortOnCache) OpenSerial();
        }
    }
    public class Text_Value_Pair<T>//自己定义的文本-值对泛型
    {
        public string TextField { get; set; }
        public T ValueField { get; set; }
    }
}