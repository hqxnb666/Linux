<%@ Page Language="C#" AutoEventWireup="true" CodeFile="Default.aspx.cs" Inherits="_Default" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
<head id="Head1" runat="server">
    <title></title>
    <style type="text/css">
        .style1
        {
            height: 6px;
        }
        .style2
        {
            height: 5px;
            width: 218px;
        }
        .style3
        {
            height: 6px;
            width: 218px;
        }
        .style4
        {
            height: 5px;
        }
        .style6
        {
            width: 218px;
            height: auto; /* 确保高度自适应内容 */
        }
        .radioButtonListStyle
        {
            display: inline-block;
            vertical-align: middle;
            line-height: 20px; /* 设置行高以垂直居中 */
        }
    </style>
</head>
<body>
    <form id="form1" runat="server">
 <table>
    <caption style="font-family: 微软雅黑; font-size: large; font-weight: bold">用户信息</caption>
    <tr>
        <td class="style4" bgcolor="#669999" style="padding: 4px;"> 用户名：</td>
        <td class="style2" bgcolor="#FFFFCC" style="padding: 4px;">
            <asp:TextBox ID="TextBox1" runat="server"></asp:TextBox>
        </td>
    </tr>
    <tr>
        <td class="style1" bgcolor="#669999" style="padding: 4px; height: 5px;"> 密码：</td>
        <td class="style2" bgcolor="#FFFFCC" style="padding: 4px;">
            <asp:TextBox ID="TextBox2" runat="server" TextMode="Password"></asp:TextBox>
        </td>
    </tr>
    <tr>
        <td bgcolor="#669999" style="padding: 4px;"> 性别：</td>
        <td class="style6" bgcolor="#FFFFCC" style="padding: 4px;">
            <asp:RadioButtonList ID="RadioButtonList1" runat="server" 
                 Width="193px" 
                 onselectedindexchanged="RadioButtonList1_SelectedIndexChanged" 
                 RepeatDirection="Horizontal" CssClass="radioButtonListStyle">
                <asp:ListItem Value="男">男</asp:ListItem>
                <asp:ListItem Value="女">女</asp:ListItem>
            </asp:RadioButtonList><br />
        </td>
    </tr>
    <tr>
        <td class="style1"> </td>
        <td class="style3" style="padding: 4px;">
            &nbsp;&nbsp;&nbsp;&nbsp;<asp:Button ID="Button1" runat="server" onclick="Button1_Click" style="height: 21px" Text="提交" />
            &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
            &nbsp;&nbsp;&nbsp; &nbsp;
        </td>
    </tr>
</table>

    </form>
</body>
</html>
