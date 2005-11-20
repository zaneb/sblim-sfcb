/*
 * Created on 16.02.2005
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 *
 * CIMDriver.java
 *
 * (C) Copyright IBM Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Sebastian Bentele <seyrich@de.ibm.com>
 *
 * Description: Implementaion of the interface Driver for the CIM-JDBC
 * 
 *
 * 
 *
 */

package com.ibm.wbem.jdbc;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.sql.Connection;
import java.sql.Driver;
import java.sql.DriverPropertyInfo;
import java.sql.SQLException;
import java.util.Properties;

/**
 * @author bentele
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class CIMDriver implements Driver {

    public static final int VERSION = 1; 
    public final static int MINVERSION = 0;
	public static final String NAME = "CIM-JDBC for sfcb";
    public static String VERSIONSTRING = VERSION+"."+MINVERSION;
   

	/**
	 * Driver will be registered on class loading
	 */
	static 
	{
			
			try
			{
					java.sql.DriverManager.registerDriver(new CIMDriver());
			}
			catch (SQLException e)
			{
					throw new RuntimeException("CIM-Driver: Can't register driver!");
			}
	}
	
	
	public CIMDriver() throws SQLException{
	    //System.out.println("Driver-Konstruktor");
	}
	
	
	/* (non-Javadoc)
	 * @see java.sql.Driver#getMajorVersion()
	 */
	public int getMajorVersion() {
		return VERSION;
	}

	/* (non-Javadoc)
	 * @see java.sql.Driver#getMinorVersion()
	 */
	public int getMinorVersion() {
		return MINVERSION;
	}

	/* (non-Javadoc)
	 * @see java.sql.Driver#jdbcCompliant()
	 */
	public boolean jdbcCompliant() {
		// TODO Auto-generated method stub
		return false;
	}

	/* 
	 * @see java.sql.Driver#acceptsURL(java.lang.String)
	 * arg0: <ip-adresse>[:port]
	 */
	public boolean acceptsURL(String arg0) throws SQLException {
		boolean acc = false;
		try {
			Socket s = getConSocket(arg0);
			BufferedReader in = new BufferedReader(new InputStreamReader(s.getInputStream()));
			PrintWriter out = new PrintWriter(s.getOutputStream());
			
			out.println("1 1 ");out.flush();
			acc = in.readLine().equals("1 1 1");
			out.println("1 2\n");out.flush();	
			
			in.close();out.close();s.close();
		} catch (UnknownHostException e) {
			throw new SQLException("UnkownHost: "+arg0);
		} catch (IOException e) {
			throw new SQLException();
		} catch (NumberFormatException e){
			throw new SQLException("Cannot parse String to int\n"+e.toString());
		}
		return acc;
	}

	private Socket getConSocket(String url) throws UnknownHostException, IOException, NumberFormatException{
		
		int c, port;
		String addr;
		if((c=url.indexOf(":"))>0){
			addr = url.substring(0,c);
			port = Integer.parseInt(url.substring(c+1,url.length()));
		}
		else{
			port = 5980;
			addr = url;
		}
		InetAddress serveraddr = InetAddress.getByName(addr);
		return new Socket(serveraddr, port);
	}
	
	/* (non-Javadoc)
	 * @see java.sql.Driver#connect(java.lang.String, java.util.Properties)
	 */
	public Connection connect(String arg0, Properties arg1) throws SQLException {
		Socket s = null;
		try {
			s = getConSocket(arg0);
			return new CIMConnection(s);
		} catch (UnknownHostException e) {
			throw new SQLException();
		} catch (IOException e) {
			throw new SQLException();
		} catch (NumberFormatException e){
			throw new SQLException();
		}
	}

	/* (non-Javadoc)
	 * @see java.sql.Driver#getPropertyInfo(java.lang.String, java.util.Properties)
	 */
	public DriverPropertyInfo[] getPropertyInfo(String arg0, Properties arg1) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}
	public static void main(String[] args){
		try {
			System.out.println((new CIMDriver().acceptsURL("127.0.0.1:5980")));
			Connection con = new CIMDriver().connect("127.0.0.1:5980",null);
			con.getMetaData();
			con.close();
		} catch (SQLException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		String a = "varchar(4987)";
		System.out.println(a.substring(a.indexOf("(")+1,a.length()-1));
		System.out.println(a.toUpperCase().substring(0,a.indexOf("(")));
		a = "dec(13,2)";
		System.out.println(""+(Integer.parseInt(a.substring(a.indexOf("(")+1,a.indexOf(",")))+1));
		System.out.println(""+(Integer.parseInt(a.substring(a.indexOf(",")+1,a.indexOf(")")))));
	}
	
}
