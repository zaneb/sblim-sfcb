/*
 * Created on 05.04.2005
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 *
 * HFRame.java
 *
 * (C) Copyright IBM Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Common Public License from
 * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
 *
 * Author:       Sebastian Bentele <seyrich@de.ibm.com>
 *
 * Description: A simple client for the CIM-JDBC. The result is shown with the html-renderer of JEditorPane
 * 
 *
 * 
 *
 */
package htmlviewer;

import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLWarning;
import java.sql.Statement;
import java.util.Enumeration;


import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.plaf.basic.BasicComboBoxEditor;
import javax.swing.JEditorPane;
import javax.swing.JScrollPane;

/**
 * @author bentele
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class HFrame extends JFrame {
	final static String header =  
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n" +
		"<html><head>\n" +
		"<title>CimSql-query</title>\n" +
		//"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"" +
		"</head>\n" +
		"<body alink=\"#ee0000\" bgcolor=\"#ffffff\" link=\"#0000ee\" text=\"#000000\" vlink=\"#551a8b\"><blockquote>";
	
	
	final static String startpage = "<html>	<body>	<br>	<br>Please type a command in the command line above</span><br></body></html>";
	
	final static String ft1 = "<font face=\"sans-serif\" size=\"-2\">";
	final static String ft2 = "<font face=\"sans-serif\" color=\"#ffffff\">";
	final static String ftc = "</font>";
	
	static {
		try{
			Class.forName("com.ibm.wbem.jdbc.CIMDriver");
			System.out.println("Treiber gefunden");
		}
		catch(Exception e){
			System.out.println("Hm...");
			e.printStackTrace();
		}
	}
	
	private JEditorPane browser;
	private JComboBox comandLineBox;
	private BasicComboBoxEditor cbEditor;
	private Connection con;
	private Statement stmt;
	
	public HFrame(){
		super("CIM-SQL Client");
		this.addWindowListener(new WindowAdapter(){
			public void windowClosing(WindowEvent e){
				//connection lösen
				try{
					stmt.close();//System.out.println("B");
					con.close();//System.out.println("C");
				}
				catch (Exception ex) {
					// TODO: handle exception
				}
				//history rausschreiben
				PrintWriter f;
				try {
					f = new PrintWriter(new BufferedWriter(new FileWriter("/home/seyrich/cimsql.hst")));
					for(int i=0; i<comandLineBox.getItemCount();i++)
						f.println((String)comandLineBox.getItemAt(i));
					f.close();
				} catch (Exception ex) {
					// TODO: handle exception
				}
				System.exit(0);
			}
		});
		Container contentPane = this.getContentPane();
		contentPane.setLayout(new BorderLayout());
		readHistory();

		initConnection();
		comandLineBox.setEditable(true);
		//		comandLineBox.setSelectedIndex(0);
		cbEditor = new BasicComboBoxEditor();
		cbEditor.addActionListener(new ActionListener(){
				public void actionPerformed(ActionEvent e) {
					executeQuery(cbEditor.getItem().toString());	
				}
			});
		comandLineBox.setEditor(cbEditor);
		contentPane.add(comandLineBox,"North");
		cbEditor.selectAll();
		browser = new JEditorPane("text/html",startpage);
		browser.setEditable(false);
		contentPane.add(new JScrollPane(browser),"Center");
		this.setSize(500,500);
		this.setVisible(true);
		
	}
	
	/**
	 * 
	 */
	private void executeQuery(String query) {
		boolean insert = true;
		for(int i=0; insert&&i<comandLineBox.getItemCount();i++)
			insert = !query.equals(comandLineBox.getItemAt(i).toString());
		if(insert)
			comandLineBox.addItem(query);
		cbEditor.selectAll();
		StringBuffer sb = new StringBuffer(header);
		ResultSet rs;
		try {
			

			rs = stmt.executeQuery(query);
			SQLWarning sw = stmt.getWarnings();
			
			sb.append("<table bgcolor=\"#180579\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">"+
					"<tbody>\n" +
					"<tr  bgcolor=\"#180579\">\n" +
					"<td valign=\"top\"><b>"+ft2+"query:"+ftc+"</b></td> " +
					"<td valign=\"top\"><code><font color=\"#ffffff\">"+query+"</font><code>\n" +
					" </td>\n" +
					" </tr>\n" +
					"<tr bgcolor=\"#180579\" >\n" +
					"<td valign=\"top\"><b>"+ft2+"SQLState:"+ftc+"</b></td> " +
					"<td valign=\"top\">"+ft2+sw.getSQLState()+ftc+"<br>\n" +
					" </td>\n" +
					" </tr>\n" +
					"<tr bgcolor=\"#180579\">\n" +
					"<td valign=\"top\"><b>"+ft2+"SQLWarningText:"+ftc+"</b></td>\n" +
					" <td valign=\"top\">"+ft2+sw.getMessage()+ftc+"<br>\n" +
					"</td>\n" +
					" </tr>\n" +
					"</tbody>\n" +
					"</table>\n" +
					"<br>");
			
			if(sw.getSQLState().equals("00000")){
				ResultSetMetaData rsms = rs.getMetaData();
				int col = rsms.getColumnCount();
				sb.append("<table border=\"0\" cellpadding=\"2\" cellspacing=\"2\" width=\"100%\"><tbody>\n" +
						"<tr bgcolor=\"#180579\"><td valign=\"top\"><b>"+ft1+ft2+"Name"+ftc+ftc+"</b></td>");
			    for (int i = 1; i <= col; i++)	
					sb.append("<td valign=\"top\"><b>"+ft1+ft2+rsms.getColumnName(i)+ftc+ftc+"</b></td>");
				
				sb.append("</tr><tr bgcolor=\"#ACCDFF\"><td valign=\"top\"><b>"+ft1+"Lable"+ftc+"</b></td>");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.getColumnLabel(i)+ftc+"</td>");
				
				sb.append("</tr><tr bgcolor=\"#96B3DE\"><td valign=\"top\"><b>"+ft1+"Tablename"+ftc+"</b></td>");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.getTableName(i)+ftc+"</td>");
				
				sb.append("</tr><tr bgcolor=\"#ACCDFF\"><td valign=\"top\"><b>"+ft1+"Typename"+ftc+"</b></td>");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.getColumnTypeName(i)+ftc+"</td>");
				
				sb.append("</tr><tr bgcolor=\"#96B3DE\"><td valign=\"top\"><b>"+ft1+"Type"+ftc+"</b></td>");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.getColumnType(i)+ftc+"</td>");
				
				sb.append("</tr><tr bgcolor=\"#ACCDFF\"><td valign=\"top\"><b>"+ft1+"TypeClassName"+ftc+"</b></td>");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.getColumnClassName(i)+ftc+"</td>");
				
				sb.append("</tr><tr bgcolor=\"#96B3DE\"><td valign=\"top\"><b>"+ft1+"Scale"+ftc+"</b></td>");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.getScale(i)+ftc+"</td>"); 
				
				sb.append("</tr><tr bgcolor=\"#ACCDFF\"><td valign=\"top\"><b>"+ft1+"Precision"+ftc+"</b></td>");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.getPrecision(i)+ftc+"</td>");
				
				sb.append("</tr><tr bgcolor=\"#96B3DE\"><td valign=\"top\"><b>"+ft1+"Nullable"+ftc+"</b></td>");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.isNullable(i)+ftc+"</td>");
				
				sb.append("</tr><tr bgcolor=\"#ACCDFF\"><td valign=\"top\"><b>"+ft1+"Readonly"+ftc+"</b></td>");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.isReadOnly(i)+ftc+"</td>");
				
				sb.append("</tr><tr bgcolor=\"#96B3DE\"><td valign=\"top\"><b>"+ft1+"Signed"+ftc+"</b></td>");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.isSigned(i)+ftc+"</td>");
				
				sb.append("</tr><tr bgcolor=\"#ACCDFF\"><td valign=\"top\"><b>"+ft1+"Writeable"+ftc+"</b></td>");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.isWritable(i)+ftc+"</td>");
				
				sb.append("</tr><tr bgcolor=\"#ACCDFF\"><td valign=\"top\"><b>"+ft1+"Displaysize"+ftc+"</b></td>\n");
				for (int i = 1; i <= col; i++)		
					sb.append("<td valign=\"top\">"+ft1+rsms.getColumnDisplaySize(i)+ftc+"</td>\n");
				int j=1;
				String[] color={"bgcolor = \"#A5C5F5\"","bgcolor = \"#96B3DE\""};
				while (rs.next()) {
					sb.append("</tr><tr><td "+color[j%2]+" valign=\"top\">"+j+"</td>\n");
					for (int i = 1; i <= col; i++) {
						sb.append("<td "+color[j%2]+" valign=\"top\">"+ft1+rs.getString(i)+ftc+"</td>\n");
					}
					j++;
					
				}
				
				sb.append("</tr></tbody></table>");

				int jj=0;
				
				System.out.println("Es tut");
				rs.close();//System.out.println("A");
			}
		}
		catch(SQLWarning w){
			try{
				SQLWarning sw = stmt.getWarnings();
			
			
				sb.append("<table bgcolor=\"#ffffff\" border=\"0\" cellpadding=\"2\" cellspacing=\"0\">"+
					"<tbody>\n" +
					"<tr >\n" +
					"<td valign=\"top\"><b>SQLState:</b></td> " +
					"<td valign=\"top\">"+sw.getSQLState()+"<br>\n" +
					" </td>\n" +
					" </tr>\n" +
					"<tr>\n" +
					"<td valign=\"top\"><b>SQLWarningText:</b></td>\n" +
					" <td valign=\"top\"><pre>"+ sw.getMessage()+"</pre><br>\n" +
					"</td>\n" +
					" </tr>\n" +
					"</tbody>\n" +
					"</table>\n" +
					"<br>");
			}catch(Exception e){}
		}
		catch (Exception e) {
			sb.append("Exception:<br>");
			StackTraceElement[] ste = e.getStackTrace();
			for(int i=0;i<ste.length;i++)
				sb.append(ste[i].toString()+"<br>");
			 
		}
		sb.append("</blockquote></body></html>");
		PrintWriter f;
		try {
			f = new PrintWriter(new BufferedWriter(new FileWriter("/home/seyrich/output.html")));
			
				f.println(sb);
			f.close();
		} catch (Exception ex) {
			// TODO: handle exception
		}
		//System.out.println("Es tut");
		browser.setText(sb.toString());
	}

	/**
	 * 
	 */
	private void initConnection() {
		try{
			Enumeration e = DriverManager.getDrivers();
		
			System.out.println("Installierte Driver:");
			while(e.hasMoreElements()){
				System.out.println("\t"+(e.nextElement()).toString());
			}
			con = DriverManager.getConnection("127.0.0.1:5980");
			stmt = con.createStatement();
		}
		catch (Exception e) {
			System.out.println("Fehler");
			e.printStackTrace();
		}
	}
	
	private void readHistory(){
		BufferedReader f;
		String line;
		if(comandLineBox == null)
			comandLineBox = new JComboBox();
		try{
			f = new BufferedReader(new FileReader("/home/seyrich/cimsql.hst"));
			while((line = f.readLine()) != null)
				comandLineBox.addItem(line);
			f.close();
		}
		catch (IOException e) {
			// TODO: handle exception
		    	comandLineBox.addItem("");
		}
		
	}
	
	public static void main(String[] args) {
		new HFrame();

	}
}
