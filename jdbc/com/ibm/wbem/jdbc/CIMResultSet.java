/*
 * Created on 16.02.2005
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 *
 * CIMResultSet.java
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
 * Description: Implementaion of the interface ResultSet for the CIM-JDBC
 * 
 *
 * 
 *
 */

package com.ibm.wbem.jdbc;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.Reader;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.net.Socket;
import java.net.URL;
import java.sql.Array;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.Date;
import java.sql.Ref;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.sql.Statement;
import java.sql.Time;
import java.sql.Timestamp;
import java.sql.Types;
import java.util.Calendar;
import java.util.Map;
import java.util.Properties;
import java.util.Vector;

/**
 * @author bentele
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class CIMResultSet implements ResultSet {

	private Socket s;
	//private boolean isLast = true;
	private String[] currentRow;
	private String[] allRows;
	private int rowCount = 1;
	private BufferedReader in;
	private PrintWriter out;
	private CIMStatement stmt;
	private Properties metaProp;
	private int maxcol;
	private CIMResultSetMetaData rsmd;
	private int cursorpos;
	
	private int maxFieldSize;
	private int maxRows;
	private int fetchSize;
	private Vector warnBuffer;
    private boolean wasNull;
	/**
	 * @param s
	 * @throws SQLException
	 */
	public CIMResultSet(Socket s, CIMStatement stmt, BufferedReader in, PrintWriter out) throws SQLException {
		this.s = s;
		this.stmt = stmt;
		this.in = in;
		this.out = out;
		String prop = "";
		if(stmt!=null){
			maxFieldSize = stmt.getMaxFieldSize();
			maxRows = stmt.getMaxRows();
		}

		try {
			StringBuffer sb = new StringBuffer();
			do{
				//System.out.println(".");
				
				prop = in.readLine();
				sb.append(prop);
				
			}while(!prop.endsWith("$$"));
			prop = sb.toString();
			prop = prop.substring(0,prop.length()-2);
			//System.out.println("prop>>>\n"+prop+"\n<<<");
			//System.out.println(prop);
		} catch (IOException e) {
			throw new SQLException();
		}
		/*try {
			in = new BufferedReader(new InputStreamReader(s.getInputStream()));
			do{
				prop += in.readLine();
				
			}while(!prop.endsWith("$$"));
		System.out.println("C");
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
			new SQLException("IOError while reading Resultsetmetadata");
		}*/
	
		try {
			String rs;
			StringBuffer sb = new StringBuffer();
			int count = 0;
			int cont = 0;
			do{
				//System.out.println(".");
				
				rs = in.readLine();
				if(maxRows>0&&count>=maxRows){
					if(cont>0&&!rs.endsWith("$$")){
						sb.append("$$");
						
					}
					cont++;
					continue;
				}
				else{
					sb.append(rs);
					sb.append("::");
				
				}
				count++;
			}while(!rs.endsWith("$$"));
			rs = sb.toString();
			//System.out.println("prop>>>\n"+rs+"\n<<<");
			rs = rs.substring(0,rs.length()-4);
			allRows = rs.split("::");
			if(cont>0)
				warnBuffer.addElement(new SQLWarning("Einige Spalten wurden verworfen: Ergebnismenge groessera als maxRows"));	
				rsmd = new CIMResultSetMetaData(prop,allRows);
			//System.out.println(prop);
		} catch (IOException e) {
			throw new SQLException();
		}
		
		cursorpos = -1;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getConcurrency()
	 */
	public int getConcurrency() throws SQLException {
		return CONCUR_READ_ONLY ;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getFetchDirection()
	 */
	public int getFetchDirection() throws SQLException {
		// TODO Auto-generated method stub
		return FETCH_UNKNOWN ;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getFetchSize()
	 */
	public int getFetchSize() throws SQLException {
		
		return allRows.length;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getRow()
	 */
	public int getRow() throws SQLException {
		return cursorpos;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getType()
	 */
	public int getType() throws SQLException {
		return ResultSet.TYPE_SCROLL_INSENSITIVE;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#afterLast()
	 */
	public void afterLast() throws SQLException {
		cursorpos = allRows.length;
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#beforeFirst()
	 */
	public void beforeFirst() throws SQLException {
		cursorpos = -1;
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#cancelRowUpdates()
	 */
	public void cancelRowUpdates() throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#clearWarnings()
	 */
	public void clearWarnings() throws SQLException {
		warnBuffer.clear();
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#close()
	 */
	public void close() throws SQLException {
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#deleteRow()
	 */
	public void deleteRow() throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#insertRow()
	 */
	public void insertRow() throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#moveToCurrentRow()
	 */
	public void moveToCurrentRow() throws SQLException {
		//nothing to do
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#moveToInsertRow()
	 */
	public void moveToInsertRow() throws SQLException {
		//nothing to to 
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#refreshRow()
	 */
	public void refreshRow() throws SQLException {
		//nothing to do
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateRow()
	 */
	public void updateRow() throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#first()
	 */
	public boolean first() throws SQLException {
		cursorpos = 0;
		if(current())
			return true;
		cursorpos = -1;
		return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#isAfterLast()
	 */
	public boolean isAfterLast() throws SQLException {
		// TODO Auto-generated method stub
		return cursorpos>=allRows.length;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#isBeforeFirst()
	 */
	public boolean isBeforeFirst() throws SQLException {
		// TODO Auto-generated method stub
		return cursorpos<0;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#isFirst()
	 */
	public boolean isFirst() throws SQLException {
		// TODO Auto-generated method stub
		return cursorpos == 0;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#isLast()
	 */
	public boolean isLast() throws SQLException {
		return cursorpos==allRows.length-1;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#last()
	 */
	public boolean last() throws SQLException {
		cursorpos=allRows.length-1;
		return true;
	}

	/*
	 * Erweiterung der API
	 */
	public boolean current(){
		
		if(cursorpos>=allRows.length||allRows.length==0)
			return false;
		if(allRows[cursorpos].startsWith("**EMPTYSET**"))
			return false;
		currentRow = allRows[cursorpos].split(";");
		return true;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#next()
	 */
	public boolean next() throws SQLException {
		cursorpos++;
		return current();
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#previous()
	 */
	public boolean previous() throws SQLException {
		cursorpos--;
		return current();
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#rowDeleted()
	 */
	public boolean rowDeleted() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#rowInserted()
	 */
	public boolean rowInserted() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#rowUpdated()
	 */
	public boolean rowUpdated() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#wasNull()
	 */
	public boolean wasNull() throws SQLException {//to be done...
	    return wasNull;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getByte(int)
	 */
	public byte getByte(int column) throws SQLException {
	    if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return 0;
	}
	    wasNull = false;   
	    return (byte)Integer.parseInt(currentRow[column-1]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getDouble(int)
	 */
	public double getDouble(int  column) throws SQLException {
	    if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return 0;
	    }
	    wasNull = false; 
		return Double.parseDouble(currentRow[column-1]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getFloat(int)
	 */
	public float getFloat(int  column) throws SQLException {
	    if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return 0;
	    }
	    wasNull = false; 
		return Float.parseFloat(currentRow[column-1]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getInt(int)
	 */
	public int getInt(int column) throws SQLException {
	    if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return 0;
	    }
	    wasNull = false; 
		return Integer.parseInt(currentRow[column-1]);
	}
	
	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getLong(int)
	 */
	public long getLong(int  column) throws SQLException {
	    if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return 0;
	    }
	    wasNull = false; 
		return Long.parseLong(currentRow[column-1]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getShort(int)
	 */
	public short getShort(int  column) throws SQLException {
	    if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return 0;
	    }
	    wasNull = false; 
		return Short.parseShort(currentRow[column-1]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#setFetchDirection(int)
	 */
	public void setFetchDirection(int arg0) throws SQLException {
		//does nothing...
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#setFetchSize(int)
	 */
	public void setFetchSize(int arg0) throws SQLException {
		//does nothing
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateNull(int)
	 */
	public void updateNull(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#absolute(int)
	 */
	public boolean absolute(int pos) throws SQLException {
		boolean res;
		if(pos>=0){
			cursorpos = pos-1;
			res = current();
			if(!res)
				cursorpos = -1;
		}
		else{
			cursorpos = allRows.length-pos;
			res = current();
			if(!res)
				cursorpos = allRows.length;
		}
		
		return res;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBoolean(int)
	 */
    public boolean getBoolean(int column) throws SQLException {
	if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
	    wasNull = true;
		return false;
	}
	wasNull = false; 
	return currentRow[column-1].equalsIgnoreCase("TRUE");
    }

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#relative(int)
	 */
	public boolean relative(int offset) throws SQLException {
		
		return absolute(cursorpos+offset);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBytes(int)
	 */
	public byte[] getBytes(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateByte(int, byte)
	 */
	public void updateByte(int arg0, byte arg1) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateDouble(int, double)
	 */
	public void updateDouble(int arg0, double arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateFloat(int, float)
	 */
	public void updateFloat(int arg0, float arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateInt(int, int)
	 */
	public void updateInt(int arg0, int arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateLong(int, long)
	 */
	public void updateLong(int arg0, long arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateShort(int, short)
	 */
	public void updateShort(int arg0, short arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateBoolean(int, boolean)
	 */
	public void updateBoolean(int arg0, boolean arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateBytes(int, byte[])
	 */
	public void updateBytes(int arg0, byte[] arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getAsciiStream(int)
	 */
	public InputStream getAsciiStream(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBinaryStream(int)
	 */
	public InputStream getBinaryStream(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getUnicodeStream(int)
	 */
	public InputStream getUnicodeStream(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateAsciiStream(int, java.io.InputStream, int)
	 */
	public void updateAsciiStream(int arg0, InputStream arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateBinaryStream(int, java.io.InputStream, int)
	 */
	public void updateBinaryStream(int arg0, InputStream arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getCharacterStream(int)
	 */
	public Reader getCharacterStream(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateCharacterStream(int, java.io.Reader, int)
	 */
	public void updateCharacterStream(int arg0, Reader arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getObject(int)
	 */
	public Object getObject(int column) throws SQLException {
	     if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return null;
	    }
	    wasNull = false; 
		Object obj;

		//Synchron zu CIMResultSetMetaData.getColumnClassName() halten
		switch(rsmd.getColumnType(column)){
			case Types.BIGINT: obj = new BigInteger(currentRow[column-1]); break;
			case Types.TIMESTAMP: obj = new Timestamp(Long.parseLong(currentRow[column-1])); break; 
			case Types.DATE: obj = new Date(Long.parseLong(currentRow[column-1]));  break;
			case Types.VARCHAR: obj = new String(currentRow[column-1]); break;
			case Types.CHAR: obj = new String(currentRow[column-1]); break;
			case Types.DECIMAL: obj = new BigDecimal(currentRow[column-1]); break;
			case Types.DOUBLE: obj = new Double(currentRow[column-1]); break;
			case Types.REAL: obj = new Float(currentRow[column-1]); break;
			case Types.INTEGER:obj = new Integer(currentRow[column-1]); break;
			case Types.SMALLINT: obj = new Integer(currentRow[column-1]); break;
			default: throw new SQLException();
		}
		//falls Wert ein Nullwert
		if(currentRow[column-1].equalsIgnoreCase("null"))
			return null;
		return obj;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateObject(int, java.lang.Object)
	 */
	public void updateObject(int arg0, Object arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateObject(int, java.lang.Object, int)
	 */
	public void updateObject(int arg0, Object arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getCursorName()
	 */
	public String getCursorName() throws SQLException {
		throw new SQLException("Not Implemented: ResultSet(): positioned update is not supported");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getString(int)
	 */
	public String getString(int  column) throws SQLException {
	    if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return null;
	    }
	    wasNull = false; 
		if(maxFieldSize>0&&maxFieldSize>currentRow[column].length()){
			currentRow[column] = currentRow[column].substring(0,maxFieldSize);
			warnBuffer.addElement(new SQLWarning(column+". Wert der "+(cursorpos+1)+". Zeile um "+(currentRow[column].length()-maxFieldSize)+" Zeichen beschnitten"));
		}
		return currentRow[column-1];
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateString(int, java.lang.String)
	 */
	public void updateString(int arg0, String arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getByte(java.lang.String)
	 */
	public byte getByte(String column) throws SQLException {
	    return (byte)(Integer.parseInt(currentRow[parseColnr(column)]));
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getDouble(java.lang.String)
	 */
	public double getDouble(String column) throws SQLException {
		return Double.parseDouble(currentRow[parseColnr(column)]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getFloat(java.lang.String)
	 */
	public float getFloat(String column) throws SQLException {
		return Float.parseFloat(currentRow[parseColnr(column)]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#findColumn(java.lang.String)
	 */
	public int findColumn(String column) throws SQLException {
		// das macht parseColumn wohl überflüssig ;-)
		return parseColnr(column);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getInt(java.lang.String)
	 */
	public int getInt(String column) throws SQLException {
		return Integer.parseInt(currentRow[parseColnr(column)]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getLong(java.lang.String)
	 */
	public long getLong(String column) throws SQLException {
		return Long.parseLong(currentRow[parseColnr(column)]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getShort(java.lang.String)
	 */
	public short getShort(String column) throws SQLException {
		return Short.parseShort(currentRow[parseColnr(column)]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateNull(java.lang.String)
	 */
	public void updateNull(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBoolean(java.lang.String)
	 */
	public boolean getBoolean(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBytes(java.lang.String)
	 */
	public byte[] getBytes(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateByte(java.lang.String, byte)
	 */
	public void updateByte(String arg0, byte arg1) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateDouble(java.lang.String, double)
	 */
	public void updateDouble(String arg0, double arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateFloat(java.lang.String, float)
	 */
	public void updateFloat(String arg0, float arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateInt(java.lang.String, int)
	 */
	public void updateInt(String arg0, int arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateLong(java.lang.String, long)
	 */
	public void updateLong(String arg0, long arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateShort(java.lang.String, short)
	 */
	public void updateShort(String arg0, short arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateBoolean(java.lang.String, boolean)
	 */
	public void updateBoolean(String arg0, boolean arg1) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateBytes(java.lang.String, byte[])
	 */
	public void updateBytes(String arg0, byte[] arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBigDecimal(int)
	 */
	public BigDecimal getBigDecimal(int column) throws SQLException {
	    if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return null;
	    }
	    wasNull = false;
		return new BigDecimal(currentRow[column-1]);
	
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBigDecimal(int, int)
	 */
	public BigDecimal getBigDecimal(int arg0, int arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateBigDecimal(int, java.math.BigDecimal)
	 */
	public void updateBigDecimal(int arg0, BigDecimal arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getURL(int)
	 */
	public URL getURL(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getArray(int)
	 */
	public Array getArray(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateArray(int, java.sql.Array)
	 */
	public void updateArray(int arg0, Array arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBlob(int)
	 */
	public Blob getBlob(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateBlob(int, java.sql.Blob)
	 */
	public void updateBlob(int arg0, Blob arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getClob(int)
	 */
	public Clob getClob(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateClob(int, java.sql.Clob)
	 */
	public void updateClob(int arg0, Clob arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getDate(int)
	 */
	public Date getDate(int column) throws SQLException {
	    if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return null;
	    }
	    wasNull = false;
		return new Date(Long.parseLong(currentRow[column-1]));
	}
	
	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateDate(int, java.sql.Date)
	 */
	public void updateDate(int arg0, Date arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getRef(int)
	 */
	public Ref getRef(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateRef(int, java.sql.Ref)
	 */
	public void updateRef(int arg0, Ref arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getMetaData()
	 */
	public ResultSetMetaData getMetaData() throws SQLException {
		// TODO Auto-generated method stub
		return rsmd;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getWarnings()
	 */
	public SQLWarning getWarnings() throws SQLException {
		// der sfcb liefert nur fehler also gibts keine warnings..
		Object o = warnBuffer.remove(0);
		if(o==null)
			return null;
		else
			return (SQLWarning)o;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getStatement()
	 */
	public Statement getStatement() throws SQLException {
		// TODO Auto-generated method stub
		return stmt;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getTime(int)
	 */
	public Time getTime(int column) throws SQLException {
	    if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return null;
	    }
	    wasNull = false;
		return new Time(Long.parseLong(currentRow[column-1]));
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateTime(int, java.sql.Time)
	 */
	public void updateTime(int arg0, Time arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getTimestamp(int)
	 */
	public Timestamp getTimestamp(int column) throws SQLException {
	    if(currentRow[column-1].equals("UNDEF")||currentRow[column-1].equals("NULL")){
		wasNull = true;
		return null;
	    }
	    wasNull = false;
		return new Timestamp(Long.parseLong(currentRow[column-1]));
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateTimestamp(int, java.sql.Timestamp)
	 */
	public void updateTimestamp(int arg0, Timestamp arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getAsciiStream(java.lang.String)
	 */
	public InputStream getAsciiStream(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBinaryStream(java.lang.String)
	 */
	public InputStream getBinaryStream(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getUnicodeStream(java.lang.String)
	 */
	public InputStream getUnicodeStream(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateAsciiStream(java.lang.String, java.io.InputStream, int)
	 */
	public void updateAsciiStream(String arg0, InputStream arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateBinaryStream(java.lang.String, java.io.InputStream, int)
	 */
	public void updateBinaryStream(String arg0, InputStream arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getCharacterStream(java.lang.String)
	 */
	public Reader getCharacterStream(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateCharacterStream(java.lang.String, java.io.Reader, int)
	 */
	public void updateCharacterStream(String arg0, Reader arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getObject(java.lang.String)
	 */
	public Object getObject(String column) throws SQLException {
		int colnr = parseColnr(column);
		return getObject(colnr);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateObject(java.lang.String, java.lang.Object)
	 */
	public void updateObject(String arg0, Object arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateObject(java.lang.String, java.lang.Object, int)
	 */
	public void updateObject(String arg0, Object arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getObject(int, java.util.Map)
	 */
	public Object getObject(int arg0, Map arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getString(java.lang.String)
	 */
	public String getString(String column) throws SQLException {
		return currentRow[parseColnr(column)];
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateString(java.lang.String, java.lang.String)
	 */
	public void updateString(String arg0, String arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBigDecimal(java.lang.String)
	 */
	public BigDecimal getBigDecimal(String column) throws SQLException {
		return new BigDecimal(currentRow[parseColnr(column)]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBigDecimal(java.lang.String, int)
	 */
	public BigDecimal getBigDecimal(String arg0, int arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateBigDecimal(java.lang.String, java.math.BigDecimal)
	 */
	public void updateBigDecimal(String arg0, BigDecimal arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getURL(java.lang.String)
	 */
	public URL getURL(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getArray(java.lang.String)
	 */
	public Array getArray(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateArray(java.lang.String, java.sql.Array)
	 */
	public void updateArray(String arg0, Array arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getBlob(java.lang.String)
	 */
	public Blob getBlob(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateBlob(java.lang.String, java.sql.Blob)
	 */
	public void updateBlob(String arg0, Blob arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getClob(java.lang.String)
	 */
	public Clob getClob(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateClob(java.lang.String, java.sql.Clob)
	 */
	public void updateClob(String arg0, Clob arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getDate(java.lang.String)
	 */
	public Date getDate(String column) throws SQLException {
		//Auf CIMOM-Date-Format achten!
		return new Date(Long.parseLong(currentRow[parseColnr(column)]));
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateDate(java.lang.String, java.sql.Date)
	 */
	public void updateDate(String arg0, Date arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getDate(int, java.util.Calendar)
	 */
	public Date getDate(int arg0, Calendar arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getRef(java.lang.String)
	 */
	public Ref getRef(String arg0) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateRef(java.lang.String, java.sql.Ref)
	 */
	public void updateRef(String arg0, Ref arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getTime(java.lang.String)
	 */
	public Time getTime(String column) throws SQLException {
//		 An das Datenformat des CIMOMS anpassen!!!!
		return new Time(Long.parseLong(currentRow[parseColnr(column)]));
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateTime(java.lang.String, java.sql.Time)
	 */
	public void updateTime(String arg0, Time arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getTime(int, java.util.Calendar)
	 */
	public Time getTime(int arg0, Calendar arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getTimestamp(java.lang.String)
	 */
	public Timestamp getTimestamp(String column) throws SQLException {
		return new Timestamp(Integer.parseInt(currentRow[parseColnr(column)]));
	}

	/**
	 * @param column
	 * @return
	 */
	private int parseColnr(String column) throws SQLException {
		int colnr=-1;
		try{
			colnr = Integer.parseInt(metaProp.getProperty(column));
		}
		catch(NumberFormatException e){
			throw new SQLException("no column "+column);
		}
		if(colnr<0||colnr>=maxcol)
			throw new SQLException("no column "+column);
		return colnr;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#updateTimestamp(java.lang.String, java.sql.Timestamp)
	 */
	public void updateTimestamp(String arg0, Timestamp arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getTimestamp(int, java.util.Calendar)
	 */
	public Timestamp getTimestamp(int arg0, Calendar arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getObject(java.lang.String, java.util.Map)
	 */
	public Object getObject(String arg0, Map arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getDate(java.lang.String, java.util.Calendar)
	 */
	public Date getDate(String arg0, Calendar arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getTime(java.lang.String, java.util.Calendar)
	 */
	public Time getTime(String arg0, Calendar arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSet#getTimestamp(java.lang.String, java.util.Calendar)
	 */
	public Timestamp getTimestamp(String arg0, Calendar arg1) throws SQLException {
		throw new SQLException("Not Implemented: ResultSet()");
	}

}
