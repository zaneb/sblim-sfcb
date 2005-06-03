/*
 * Created on 16.02.2005
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
package com.ibm.wbem.jdbc;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.sql.Statement;

/**
 * @author seyrich
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class CIMStatement implements Statement {

	private Socket s;
	private BufferedReader in;
	private PrintWriter out;
	private CIMConnection con;
	private ResultSet rs;
	private SQLWarning sw;
	private int fetchDir;
	private int maxFieldSize = 0;
	private int maxRows = 0;
	private int fetchSize = 0;

	/**
	 * @param s
	 * @param connectio
	 * @throws SQLException
	 */
	public CIMStatement(Socket s, CIMConnection con) throws SQLException {
		this.s = s;
		this.con = con;
		try {
			in = new BufferedReader(new InputStreamReader(s.getInputStream()));
			out = new PrintWriter(s.getOutputStream());
		} catch (IOException e) {
			throw new SQLException();
		}
		fetchDir = ResultSet.FETCH_FORWARD;
	}

	/**
	 * @param s2
	 * @param connection
	 */
	

	/* (non-Javadoc)
	 * @see java.sql.Statement#getFetchDirection()
	 */
	public int getFetchDirection() throws SQLException {
		return fetchDir;
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getFetchSize()
	 */
	public int getFetchSize() throws SQLException {
		return fetchSize;//no limit
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getMaxFieldSize()
	 */
	public int getMaxFieldSize() throws SQLException {
		return maxFieldSize;//no limit
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getMaxRows()
	 */
	public int getMaxRows() throws SQLException {
		return maxRows;//no limit
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getQueryTimeout()
	 */
	public int getQueryTimeout() throws SQLException {
		// TODO Auto-generated method stub
		return 0;//no limit
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getResultSetConcurrency()
	 */
	public int getResultSetConcurrency() throws SQLException {
		return ResultSet.CONCUR_READ_ONLY;
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getResultSetHoldability()
	 */
	public int getResultSetHoldability() throws SQLException {
		throw new SQLException("Not Implemented: Statement()");//commit not spported
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getResultSetType()
	 */
	public int getResultSetType() throws SQLException {
		return ResultSet.TYPE_SCROLL_INSENSITIVE;
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getUpdateCount()
	 */
	public int getUpdateCount() throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#cancel()
	 */
	public void cancel() throws SQLException {
		//beide muessten das unterstuetzen. jdbc und cimom!!
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#clearBatch()
	 */
	public void clearBatch() throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#clearWarnings()
	 */
	public void clearWarnings() throws SQLException {
		// nothing to do...
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#close()
	 */
	public void close() throws SQLException {
		//Pipe darf nicht closed werden, sonst ist der ganze Socket tot!
		rs = null;
		sw = null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getMoreResults()
	 */
	public boolean getMoreResults() throws SQLException {
		return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#executeBatch()
	 */
	public int[] executeBatch() throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#setFetchDirection(int)
	 */
	public void setFetchDirection(int dir) throws SQLException {
		fetchDir = dir;
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#setFetchSize(int)
	 */
	public void setFetchSize(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#setMaxFieldSize(int)
	 */
	public void setMaxFieldSize(int size) throws SQLException {
		maxFieldSize = size;
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#setMaxRows(int)
	 */
	public void setMaxRows(int maxRows) throws SQLException {
		this.maxRows = maxRows;
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#setQueryTimeout(int)
	 */
	public void setQueryTimeout(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getMoreResults(int)
	 */
	public boolean getMoreResults(int arg0) throws SQLException {
		return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#setEscapeProcessing(boolean)
	 */
	public void setEscapeProcessing(boolean arg0) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#executeUpdate(java.lang.String)
	 */
	public int executeUpdate(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#addBatch(java.lang.String)
	 */
	public void addBatch(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#setCursorName(java.lang.String)
	 */
	public void setCursorName(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#execute(java.lang.String)
	 */
	public boolean execute(String sql) throws SQLException {
		try {
			rs = executeQuery(sql);
			return true;
		} catch (Exception e) {
			return false;
		}
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#executeUpdate(java.lang.String, int)
	 */
	public int executeUpdate(String arg0, int arg1) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#execute(java.lang.String, int)
	 */
	public boolean execute(String arg0, int arg1) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#executeUpdate(java.lang.String, int[])
	 */
	public int executeUpdate(String arg0, int[] arg1) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#execute(java.lang.String, int[])
	 */
	public boolean execute(String arg0, int[] arg1) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getConnection()
	 */
	public Connection getConnection() throws SQLException {
		return con;
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getGeneratedKeys()
	 */
	public ResultSet getGeneratedKeys() throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getResultSet()
	 */
	public ResultSet getResultSet() throws SQLException {
		// TODO Auto-generated method stub
		return rs;
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#getWarnings()
	 */
	public SQLWarning getWarnings() throws SQLException {
		return sw;
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#executeUpdate(java.lang.String, java.lang.String[])
	 */
	public int executeUpdate(String arg0, String[] arg1) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#execute(java.lang.String, java.lang.String[])
	 */
	public boolean execute(String arg0, String[] arg1) throws SQLException {
		throw new SQLException("Not Implemented: Statement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.Statement#executeQuery(java.lang.String)
	 */
	public ResultSet executeQuery(String sql) throws SQLException {
		sw = null;rs = null;
		//Anfrage  abschicken
		out.println("2 "+sql+"$");out.flush();
		String inString="";
		try {
			//Antwort abwarten und analysieren
			//Fehler
			
			
			String protocollSt=in.readLine();
			boolean first = true; 
			do{
				if(!first)
					inString += "\n"+in.readLine();
				else{
					inString += in.readLine();
					first = false;
				}
			}while(!inString.endsWith("$$"));
			System.out.println(protocollSt+"<<<\n"+inString);
			inString = inString.substring(0,inString.length()-2);
			String w[] = inString.split(";");
			sw = new SQLWarning(w[1],w[0]);
			if(!protocollSt.equals("2 1")){
				System.out.println("2 0: "+protocollSt);
			
				throw sw;
			}
			else{
				
				//System.out.println("2 1 "+inString);
				//Warning einlesen
				
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			throw new SQLException("IOException reading Socket");
		}
		
		rs = new CIMResultSet(s,this,in,out);
		return rs;
	}
	protected BufferedReader getIn() {
		return in;
	}
	protected PrintWriter getOut() {
		return out;
	}
}
