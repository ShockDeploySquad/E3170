namespace SerialSendAndReceive
{
    partial class MainWin
    {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要修改
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent()
        {
            this.DisplayArea = new System.Windows.Forms.TextBox();
            this.PortLabel = new System.Windows.Forms.Label();
            this.baudSelectLabel = new System.Windows.Forms.Label();
            this.PortSelect = new System.Windows.Forms.ComboBox();
            this.BaudSelect = new System.Windows.Forms.ComboBox();
            this.MessageInput = new System.Windows.Forms.TextBox();
            this.SendBtn = new System.Windows.Forms.Button();
            this.PortControlBtn = new System.Windows.Forms.Button();
            this.StopBitsLabel = new System.Windows.Forms.Label();
            this.StopBitsSelect = new System.Windows.Forms.ComboBox();
            this.ParityLabel = new System.Windows.Forms.Label();
            this.ParitySelect = new System.Windows.Forms.ComboBox();
            this.DataBitsLabel = new System.Windows.Forms.Label();
            this.DataBitsSelect = new System.Windows.Forms.ComboBox();
            this.HexModeCheck = new System.Windows.Forms.CheckBox();
            this.SuspendLayout();
            // 
            // DisplayArea
            // 
            this.DisplayArea.Location = new System.Drawing.Point(12, 12);
            this.DisplayArea.Multiline = true;
            this.DisplayArea.Name = "DisplayArea";
            this.DisplayArea.ReadOnly = true;
            this.DisplayArea.ScrollBars = System.Windows.Forms.ScrollBars.Horizontal;
            this.DisplayArea.Size = new System.Drawing.Size(776, 329);
            this.DisplayArea.TabIndex = 1;
            // 
            // PortLabel
            // 
            this.PortLabel.AutoSize = true;
            this.PortLabel.Location = new System.Drawing.Point(9, 364);
            this.PortLabel.Name = "PortLabel";
            this.PortLabel.Size = new System.Drawing.Size(45, 15);
            this.PortLabel.TabIndex = 3;
            this.PortLabel.Text = "串口:";
            // 
            // baudSelectLabel
            // 
            this.baudSelectLabel.AutoSize = true;
            this.baudSelectLabel.Location = new System.Drawing.Point(9, 401);
            this.baudSelectLabel.Name = "baudSelectLabel";
            this.baudSelectLabel.Size = new System.Drawing.Size(60, 15);
            this.baudSelectLabel.TabIndex = 2;
            this.baudSelectLabel.Text = "波特率:";
            // 
            // PortSelect
            // 
            this.PortSelect.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.PortSelect.FormattingEnabled = true;
            this.PortSelect.Location = new System.Drawing.Point(61, 361);
            this.PortSelect.Name = "PortSelect";
            this.PortSelect.Size = new System.Drawing.Size(121, 23);
            this.PortSelect.TabIndex = 1;
            this.PortSelect.SelectedIndexChanged += new System.EventHandler(this.PortSelect_SelectedIndexChanged);
            // 
            // BaudSelect
            // 
            this.BaudSelect.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.BaudSelect.FormattingEnabled = true;
            this.BaudSelect.Location = new System.Drawing.Point(75, 397);
            this.BaudSelect.Name = "BaudSelect";
            this.BaudSelect.Size = new System.Drawing.Size(107, 23);
            this.BaudSelect.TabIndex = 2;
            this.BaudSelect.SelectedIndexChanged += new System.EventHandler(this.BaudSelect_SelectedIndexChanged);
            // 
            // MessageInput
            // 
            this.MessageInput.Location = new System.Drawing.Point(270, 359);
            this.MessageInput.Name = "MessageInput";
            this.MessageInput.Size = new System.Drawing.Size(437, 25);
            this.MessageInput.TabIndex = 0;
            // 
            // SendBtn
            // 
            this.SendBtn.Location = new System.Drawing.Point(713, 361);
            this.SendBtn.Name = "SendBtn";
            this.SendBtn.Size = new System.Drawing.Size(75, 23);
            this.SendBtn.TabIndex = 6;
            this.SendBtn.Text = "发送";
            this.SendBtn.UseVisualStyleBackColor = true;
            this.SendBtn.Click += new System.EventHandler(this.SendBtn_Click);
            // 
            // PortControlBtn
            // 
            this.PortControlBtn.Location = new System.Drawing.Point(188, 359);
            this.PortControlBtn.Name = "PortControlBtn";
            this.PortControlBtn.Size = new System.Drawing.Size(75, 23);
            this.PortControlBtn.TabIndex = 7;
            this.PortControlBtn.Text = "打开";
            this.PortControlBtn.UseVisualStyleBackColor = true;
            this.PortControlBtn.Click += new System.EventHandler(this.PortControlBtn_Click);
            // 
            // StopBitsLabel
            // 
            this.StopBitsLabel.AutoSize = true;
            this.StopBitsLabel.Location = new System.Drawing.Point(312, 402);
            this.StopBitsLabel.Name = "StopBitsLabel";
            this.StopBitsLabel.Size = new System.Drawing.Size(60, 15);
            this.StopBitsLabel.TabIndex = 8;
            this.StopBitsLabel.Text = "停止位:";
            // 
            // StopBitsSelect
            // 
            this.StopBitsSelect.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.StopBitsSelect.FormattingEnabled = true;
            this.StopBitsSelect.Location = new System.Drawing.Point(375, 397);
            this.StopBitsSelect.Name = "StopBitsSelect";
            this.StopBitsSelect.Size = new System.Drawing.Size(50, 23);
            this.StopBitsSelect.TabIndex = 9;
            this.StopBitsSelect.SelectedIndexChanged += new System.EventHandler(this.StopBitsSelect_SelectedIndexChanged);
            // 
            // ParityLabel
            // 
            this.ParityLabel.AutoSize = true;
            this.ParityLabel.Location = new System.Drawing.Point(431, 402);
            this.ParityLabel.Name = "ParityLabel";
            this.ParityLabel.Size = new System.Drawing.Size(75, 15);
            this.ParityLabel.TabIndex = 10;
            this.ParityLabel.Text = "奇偶校验:";
            // 
            // ParitySelect
            // 
            this.ParitySelect.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ParitySelect.FormattingEnabled = true;
            this.ParitySelect.Location = new System.Drawing.Point(512, 398);
            this.ParitySelect.Name = "ParitySelect";
            this.ParitySelect.Size = new System.Drawing.Size(121, 23);
            this.ParitySelect.TabIndex = 9;
            this.ParitySelect.SelectedIndexChanged += new System.EventHandler(this.ParitySelect_SelectedIndexChanged);
            // 
            // DataBitsLabel
            // 
            this.DataBitsLabel.AutoSize = true;
            this.DataBitsLabel.Location = new System.Drawing.Point(190, 401);
            this.DataBitsLabel.Name = "DataBitsLabel";
            this.DataBitsLabel.Size = new System.Drawing.Size(60, 15);
            this.DataBitsLabel.TabIndex = 10;
            this.DataBitsLabel.Text = "数据位:";
            // 
            // DataBitsSelect
            // 
            this.DataBitsSelect.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.DataBitsSelect.FormattingEnabled = true;
            this.DataBitsSelect.Location = new System.Drawing.Point(256, 397);
            this.DataBitsSelect.Name = "DataBitsSelect";
            this.DataBitsSelect.Size = new System.Drawing.Size(50, 23);
            this.DataBitsSelect.TabIndex = 9;
            this.DataBitsSelect.SelectedIndexChanged += new System.EventHandler(this.DataBitsSelect_SelectedIndexChanged);
            // 
            // HexModeCheck
            // 
            this.HexModeCheck.AutoSize = true;
            this.HexModeCheck.Location = new System.Drawing.Point(668, 401);
            this.HexModeCheck.Name = "HexModeCheck";
            this.HexModeCheck.Size = new System.Drawing.Size(120, 19);
            this.HexModeCheck.TabIndex = 14;
            this.HexModeCheck.Text = "按16进制显示";
            this.HexModeCheck.UseVisualStyleBackColor = true;
            // 
            // MainWin
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 431);
            this.Controls.Add(this.HexModeCheck);
            this.Controls.Add(this.ParitySelect);
            this.Controls.Add(this.DataBitsSelect);
            this.Controls.Add(this.StopBitsSelect);
            this.Controls.Add(this.ParityLabel);
            this.Controls.Add(this.DataBitsLabel);
            this.Controls.Add(this.StopBitsLabel);
            this.Controls.Add(this.PortControlBtn);
            this.Controls.Add(this.SendBtn);
            this.Controls.Add(this.MessageInput);
            this.Controls.Add(this.BaudSelect);
            this.Controls.Add(this.PortSelect);
            this.Controls.Add(this.baudSelectLabel);
            this.Controls.Add(this.PortLabel);
            this.Controls.Add(this.DisplayArea);
            this.Name = "MainWin";
            this.Text = "串口通讯器 - 已关闭";
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox DisplayArea;
        private System.Windows.Forms.Label PortLabel;
        private System.Windows.Forms.Label baudSelectLabel;
        private System.Windows.Forms.ComboBox PortSelect;
        private System.Windows.Forms.ComboBox BaudSelect;
        private System.Windows.Forms.TextBox MessageInput;
        private System.Windows.Forms.Button SendBtn;
        private System.Windows.Forms.Button PortControlBtn;
        private System.Windows.Forms.Label StopBitsLabel;
        private System.Windows.Forms.ComboBox StopBitsSelect;
        private System.Windows.Forms.Label ParityLabel;
        private System.Windows.Forms.ComboBox ParitySelect;
        private System.Windows.Forms.Label DataBitsLabel;
        private System.Windows.Forms.ComboBox DataBitsSelect;
        private System.Windows.Forms.CheckBox HexModeCheck;
    }
}

