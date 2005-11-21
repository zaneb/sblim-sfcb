/*
 * myCimClient.java
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
 * Contributors: see README.txt
 * 
 *
*/


package com.ibm.wbem.jdbc.client;

import java.io.*;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.*;

/**
 * Command Line Client for the CIM JDBC Interface
 * 
 * @author Martin Knipper
 */


public class myCimClient
{
	public static testDriver mydb;
	
	public myCimClient(testDriver t) 
	{
		mydb=t;
	}
	
	public static void main(String [] args)
	{
	final String initalPrompt  =   new String("CIM-JDBC ->");
	final String extendedPrompt  = new String("CIM-JDBC >>");
	
	String prompt = null;
	String line = new String();
	String wholeInput = new String();
	testDriver mydb=null;
	
	if(args.length>0)
	{
		if(args.length==1)
		{
			System.out.println("Connecting to "+args[0].toString());
			mydb = new testDriver(args[0].toString(),5988);
		}
		else
		{
			System.out.println("Connecting to "+args[0].toString()+" with Port "+args[1].toString());
			mydb = new testDriver(args[0].toString(), new Integer(args[1].toString()).intValue());
		}
	}
	else
	{
		System.out.println("Connecting to localhost (127.0.0.1)");
		mydb = new testDriver("127.0.0.1",5988); 
	}
	 
	Date start,ende;
	
	myCimClient mcl = new myCimClient(mydb);



	prompt=initalPrompt;
	BufferedReader stdin
						   = new BufferedReader(
									new InputStreamReader( System.in ) );

	while (true) 
	    {
		try 
		    {
			System.out.flush();
			System.err.flush();
			System.out.print(prompt);     
			line = stdin.readLine();					
			if (line == null)
			    continue;
			else
			    {
				Statement st = null;
				st = mydb.connection().createStatement();
				if(st==null)
				    System.err.println("st ist null");
				
				// Preparse the command for internal usage
				if(myCimClient.parse(line))
				    {
					prompt=initalPrompt;
					wholeInput="";
					continue;
				    } 
				
				String g = line.trim();
				if(g.charAt(g.length()-1)==';')					
				    {
					prompt=initalPrompt;
					wholeInput +=" "+line+" ";
					mydb.query(st,wholeInput);
					wholeInput="";
				    }
				else
				    {
					prompt=extendedPrompt;
					wholeInput += line;
					continue;
					
				    }					
			    } 
		    } 
		catch (EOFException e) 
		    {
			System.out.println("\n... Bye");
			wholeInput="";
			break;
		    } 
		catch (SQLException e) 
		    {
			System.err.println("SQL-ERR: "+e.getMessage());	e.printStackTrace();
			wholeInput="";
		    }
		catch (Exception e) 
		    {
			System.err.println("EXC-ERR: "+e.toString()+"\n"+e.getMessage());
			e.printStackTrace();				
		    }	
		System.err.flush();			
		System.out.flush();
	    }
	
	
	}

	private static boolean parse(String line) throws Exception
	{
		if(line.equals("\\q"))
		{
			System.out.println("\n... Bye");
			mydb.connection().close();
			System.exit(0);
		}
		if(line.equals("\\?"))
		{
			System.out.println("\nHELP:");
			System.out.println("-----");
			System.out.println("\\?              Show this help");
			System.out.println("\\df <filter>	METADATA -> Shows Tables matching <filter> condition -> '%' is wildcard");
			System.out.println("\\d <tablename>  METADATA -> Describe Table");
			System.out.println("\\q              Quit the Client");
			System.out.println();
			System.out.flush();
			return true;
			
		}
		if(line.equals("\\q"))
				{
					System.out.println("\n... Bye");
					mydb.connection().close();
					System.exit(0);
				}
		if(line.startsWith("\\df"))
		{
			line = line.trim();
			StringTokenizer st = new StringTokenizer(line," ");
			st.nextToken(" ");
			String search;
			if(st.hasMoreTokens())
			{
				search = st.nextToken(" ");
			}
			else
			{
				System.err.println("You have to provide a filter. For example: \\df CIM_%");
				return true;
			}
			Date start = new Date();

			ResultSet ergeb = mydb.connection().getMetaData().getTables("root/cimv2",null,search,null);
			mydb.metaQuery(ergeb,start);
			
			return true;
		}		
		if(line.startsWith("\\d"))
		{
			
			line = line.trim();
			StringTokenizer st = new StringTokenizer(line," ");
			st.nextToken(" ");
			String search;
			if(st.hasMoreTokens())
			{
				search = st.nextToken(" ");
			}
			else
			{
				System.err.println("You have to provide a table. For example: \\d cim_process");
				return true;
			}
			
			Date start = new Date();
			ResultSet ergeb = mydb.connection().getMetaData().getColumns("root/cimv2",null,search,null);
			mydb.metaQuery(ergeb,start);
			return true;
		}		
		return false;
	}

	
}

