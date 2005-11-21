/*
 * testDriver.java
 *
 * (C) Copyright IBM Corp. 2004
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Common Public License from
 * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
 *
 * Author:       Martin Knipper <knipper@de.ibm.com> 
 * Contributors:  or <cim-jdbc@mk-os.de>
 * 
 *
*/


package com.ibm.wbem.jdbc.client;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.Statement;
import java.util.Date;

/**
 * This class is responsible for the communication with the myCimClient class
 * 
 * @author Martin Knipper*
 * 
 */
public class testDriver 
{
	private Connection dbx;
	private String host=null;
	private int port;
	
	/**
	 * construct an instance
	 * @param host hostname or ip address
	 * @param _port the port
	 */
	public testDriver(String host, int _port)
	{
		port = _port;
		try
		{
			System.out.println("Loading Driver ...");
			Class.forName("com.ibm.wbem.jdbc.CIMDriver");
		}
		catch(java.lang.ClassNotFoundException e)
		{
			System.err.println("Driver not found\n");
			System.err.println(e.getMessage());
			System.exit(1);
		}
		try
		{
		    dbx = DriverManager.getConnection("127.0.0.1:5980");//jdbc:cim:http:"+host+":"+port+"");
		}
		catch(java.sql.SQLException e)
		{
			System.err.println("Connection-error "+e.toString());
			System.err.println(e.getMessage());
			System.exit(1);
		}		
		
	}	
	
	/**
	 * 
	 * @return an instance of cimConnection
	 */
	public Connection connection()
	{
				return dbx;
	}
	
	/**
	 * Process query
	 * @param st an instance of statement
	 * @param sql the SQL Query String
	 * @throws Exception
	 */
	public void query(Statement st,String sql) throws Exception
		{
		
				Date start,ende;
				start = new Date();
						   
				System.out.println("QUERY: "+sql);
				ResultSet ergeb = st.executeQuery(sql);
				outputResult(ergeb,start,st);
		}
		
		/**
		 * Diplay Metadata
		 * @param e cimResultSet
		 * @param start starting Time - instance of Date
		 * @throws Exception
		 */
		public void metaQuery(ResultSet e,Date start) throws Exception
		{
			outputResult(e,start,e.getStatement());
		}
		
		private void outputResult(ResultSet ergeb, Date start, Statement st) throws Exception
		{
				Date ende;
				if(ergeb==null)
				{
					ende = new Date();
					long diff = ende.getTime()-start.getTime();
							
					int numOfUpdates = st.getUpdateCount();
					System.out.println();
					System.out.println("Number of affected Rows: "+numOfUpdates);
					System.out.println("Duration: "+diff+" msec");
					return;
				}
				else
				{
				
				int counter=0;
				boolean ret = true;
				StringBuffer sb1 = new StringBuffer();
			
				for(int i=1;i<=ergeb.getMetaData().getColumnCount();i++)
				{
					String ColName = ergeb.getMetaData().getColumnName(i);						
					int width = ergeb.getMetaData().getColumnDisplaySize(i);	
					sb1.append(ColName);
					for(int x=0;x<(width-ColName.length());x++)
						sb1.append(" ");	
					sb1.append(" | ");
				}
				System.out.println(sb1);
				StringBuffer sb2 = new StringBuffer();
				for(int i=0;i<sb1.length()-1;i++)
				{
					sb2.append("-");
				}
				System.out.println(sb2);
			
				while(ret==true)
				{
					if(ergeb.next())
					{
						
						counter++;
						StringBuffer sb = new StringBuffer();
						
						for(int i=1;i<=ergeb.getMetaData().getColumnCount();i++)
						{
							int width = ergeb.getMetaData().getColumnDisplaySize(i);					
							
							String temp = ergeb.getString(i);
							if(ergeb.wasNull())
							{
								// Null values are displayes as -NULL- 
								temp = new String("-NULL-");
							}							
							sb.append(temp);
							
							for(int x=0;x<(width-temp.length());x++)
								sb.append(" ");	
							sb.append(" | ");
						}
						System.out.println(sb.toString());					
					}
					else
					{	
						ende = new Date();
						long diff = ende.getTime()-start.getTime();
						System.out.println("\nReturning: "+counter+" Row(s) in "+diff+" msec");
						ret=false;	
					}					
				}	
		}		
	}	
}
