/*
 * Created on 16.02.2005
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 *
 * CIMConnection.java
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
 * Description: Implementaion of the interface Connection for the CIM-JDBC
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
import java.net.Socket;
import java.sql.CallableStatement;
import java.sql.Connection;
import java.sql.DatabaseMetaData;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.sql.Savepoint;
import java.sql.Statement;
import java.util.Map;
import java.util.Properties;

/**
 * @author bentele
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class CIMConnection implements Connection{
	private BufferedReader in;
	private PrintWriter out;
	private Socket s;
	private String catalog;
	private boolean readOnly;
	private DatabaseMetaData dbMeta;
	private SQLWarning sqlWarnings;
	
	public CIMConnection(Socket s) throws IOException{
		this.s = s;
		in = new BufferedReader(new InputStreamReader(s.getInputStream()));
		out = new PrintWriter(s.getOutputStream());
		
		out.println("1 1 ");out.flush();
		if(!in.readLine().equals("1 1 1"))
			s=null;
	}
	
	/* (non-Javadoc)
	 * @see java.sql.Connection#getHoldability()
	 */
	public int getHoldability() throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: getHoldability()");
		//return 0;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#getTransactionIsolation()
	 */
	public int getTransactionIsolation() throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: getTransactionIsolation()");
		//return 0;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#clearWarnings()
	 */
	public void clearWarnings() throws SQLException {
		// TODO Auto-generated method stub
		sqlWarnings = null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#close()
	 */
	public void close() throws SQLException {
	    //System.out.println("close()");
		if(s==null)
			return;
		out.println("1 2\n");out.flush();
		//	System.out.println("1 2 gesendet");
		try {
			in.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		out.close();
		try {
			s.close();
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#commit()
	 */
	public void commit() throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: commit()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#rollback()
	 */
	public void rollback() throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: rollback()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#getAutoCommit()
	 */
	public boolean getAutoCommit() throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: getAutoCommit()");
		//return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#isClosed()
	 */
	public boolean isClosed() throws SQLException {
		
		return s==null||s.isClosed();
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#isReadOnly()
	 */
	public boolean isReadOnly() throws SQLException {
		// TODO Auto-generated method stub
		return readOnly;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#setHoldability(int)
	 */
	public void setHoldability(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: setHoldability()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#setTransactionIsolation(int)
	 */
	public void setTransactionIsolation(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: setTransactionIsolation()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#setAutoCommit(boolean)
	 */
	public void setAutoCommit(boolean arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: setAutoCommit()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#setReadOnly(boolean)
	 */
	public void setReadOnly(boolean arg0) throws SQLException {
		//sollte an die DB weitergegeben werden. Dmit auch in Selects keine Funktionen mit Seiteneffekten aufgerufen werden können
		readOnly = arg0;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#getCatalog()
	 */
	public String getCatalog() throws SQLException {
		
		return catalog;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#setCatalog(java.lang.String)
	 */
	public void setCatalog(String arg0) throws SQLException {
		
		catalog = arg0;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#getMetaData()
	 */
	public DatabaseMetaData getMetaData() throws SQLException {
		//diese Daten müssen einmalig erhoben werden. Allerdings 
		//nicht im Konstruktor, da die Daten nicht bei jeder Connection
		//vom Client angefordert werden
		if(dbMeta==null){
			out.println("3 1 ");out.flush();
			String inString=null;
			try {
				
				if(in.readLine().equals("3 1 1")){
					do{
						inString += in.readLine();
						
					}while(!inString.endsWith("$$"));
					Properties prop = new Properties();
					inString = inString.substring(0,inString.length()-2);
					String[] s = inString.split(";");
					for(int i=0; i<s.length;i++){
						String[] t = s[i].split("=");
						prop.setProperty(t[0],t[1]);
					}
					dbMeta = new CIMDatabaseMetaData(prop,this.s,this,in,out);
				}
				else
					System.out.println("Fehler ");
				//Parameterliste empfangen 
				//DatabaseMetaData erzeugen mit Liste als Argument.
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				throw new SQLException("IOException reading Socket");
			}
		}
		return dbMeta;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#getWarnings()
	 */
	public SQLWarning getWarnings() throws SQLException {
		
		return sqlWarnings;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#setSavepoint()
	 */
	public Savepoint setSavepoint() throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: setSavepoint()");
		//return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#releaseSavepoint(java.sql.Savepoint)
	 */
	public void releaseSavepoint(Savepoint arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: releaseSavepoint()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#rollback(java.sql.Savepoint)
	 */
	public void rollback(Savepoint arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: rollback()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#createStatement()
	 */
	public Statement createStatement() throws SQLException {
		return new CIMStatement(s,this);
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#createStatement(int, int)
	 */
	public Statement createStatement(int resultSetType, int resultSetConcurrency) throws SQLException {
		throw new SQLException("Not Implemented: createStatement(int resultSetType, int resultSetConcurrency)");
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#createStatement(int, int, int)
	 */
	public Statement createStatement(int resultSetType, int resultSetConcurrency, int resultSetHoldability) throws SQLException {
		throw new SQLException("Not Implemented: createStatement(int resultSetType, int resultSetConcurrency, int resultSetHoldability)");
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#getTypeMap()
	 */
	public Map getTypeMap() throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: getTypeMap()");
		//return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#setTypeMap(java.util.Map)
	 */
	public void setTypeMap(Map arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: setTypeMap()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#nativeSQL(java.lang.String)
	 */
	public String nativeSQL(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: nativeSQL()");
		//return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#prepareCall(java.lang.String)
	 */
	public CallableStatement prepareCall(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: prepareCall()");
		//return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#prepareCall(java.lang.String, int, int)
	 */
	public CallableStatement prepareCall(String arg0, int arg1, int arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: prepareCall()");
		//return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#prepareCall(java.lang.String, int, int, int)
	 */
	public CallableStatement prepareCall(String arg0, int arg1, int arg2, int arg3) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: prepareCall()");
		//return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#prepareStatement(java.lang.String)
	 */
	public PreparedStatement prepareStatement(String sql) throws SQLException {
		// TODO Auto-generated method stub
		return new CIMPreparedStatement(s,this,sql);
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#prepareStatement(java.lang.String, int)
	 */
	public PreparedStatement prepareStatement(String arg0, int arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: prepareStatement()");
		//return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#prepareStatement(java.lang.String, int, int)
	 */
	public PreparedStatement prepareStatement(String arg0, int arg1, int arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: prepareStatement()");
		//return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#prepareStatement(java.lang.String, int, int, int)
	 */
	public PreparedStatement prepareStatement(String arg0, int arg1, int arg2, int arg3) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: prepareStatement()");
		//return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#prepareStatement(java.lang.String, int[])
	 */
	public PreparedStatement prepareStatement(String arg0, int[] arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: prepareStatement()");
		//return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#setSavepoint(java.lang.String)
	 */
	public Savepoint setSavepoint(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: setSavepoint()");
		//return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#prepareStatement(java.lang.String, java.lang.String[])
	 */
	public PreparedStatement prepareStatement(String arg0, String[] arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: prepareStatement()");
		//return null;
	}

}
