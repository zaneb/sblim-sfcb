/*
 * Created on 16.02.2005
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 *
 * CIMPreparedStatement.java
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
 * Description: Implementaion of the interface PreparedStatement for the CIM-JDBC
 * 
 *
 * 
 *
 */

package com.ibm.wbem.jdbc;

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.net.Socket;
import java.net.URL;
import java.sql.Array;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.Date;
import java.sql.ParameterMetaData;
import java.sql.PreparedStatement;
import java.sql.Ref;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Time;
import java.sql.Timestamp;
import java.sql.Types;
import java.util.Calendar;

/**
 * @author bentele
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class CIMPreparedStatement extends CIMStatement implements PreparedStatement{

	private String sql;
	protected String[] values;
	/**
	 * @param connection
	 * @param s
	 * @param arg0
	 */
	public CIMPreparedStatement(Socket s, CIMConnection connection, String sql) throws SQLException{
		super(s,connection);
		this.sql = sql;
		char c[] = sql.toCharArray();
		int count = 0;
		for (int i = 0; i < c.length; i++) {
			if(c[i]=='?')
				count++;
		}
		values = new String[count];
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#executeUpdate()
	 */
	public int executeUpdate() throws SQLException {
		return super.executeUpdate(formatSQL());
	}
		
	public String toString(){
		try {
			return formatSQL();
		} catch (Exception e) {
			return e.toString();
		}
	}
	
	/**
	 * @return
	 */
	private String formatSQL() throws SQLException {
		String res = sql;
		//System.out.println(sql);
		for (int i = 0; i < values.length; i++) {
			if(values[i]==null)
				throw new SQLException(i+". value is not set");
			//System.out.println(values[i]);
			res = res.replaceFirst("\\?",values[i]);
		}
		return res;
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#addBatch()
	 */
	public void addBatch() throws SQLException {
		throw new SQLException("Not Implemented: CallableStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#clearParameters()
	 */
	public void clearParameters() throws SQLException {
		for (int i = 0; i < values.length; i++) {
			values[i]=null;
		}
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#execute()
	 */
	public boolean execute() throws SQLException {
		return super.execute(formatSQL());
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setByte(int, byte)
	 */
	public void setByte(int parameterIndex, byte arg1) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setDouble(int, double)
	 */
	public void setDouble(int parameterIndex, double val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val+"";
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setFloat(int, float)
	 */
	public void setFloat(int parameterIndex, float val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val+"";
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setInt(int, int)
	 */
	public void setInt(int parameterIndex, int val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val+"";
		
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setNull(int, int)
	 */
	public void setNull(int parameterIndex, int val) throws SQLException {
		if(Types.NULL!=val)
			throw new SQLException(val+" doesn't identify a null value");
		values[parameterIndex-1] = "NULL";
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setLong(int, long)
	 */
	public void setLong(int parameterIndex, long val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val+"";
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setShort(int, short)
	 */
	public void setShort(int parameterIndex, short val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val+"";
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setBoolean(int, boolean)
	 */
	public void setBoolean(int parameterIndex, boolean val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val ? "true":"false";
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setBytes(int, byte[])
	 */
	public void setBytes(int parameterIndex, byte[] val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = new String(val);
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setAsciiStream(int, java.io.InputStream, int)
	 */
	public void setAsciiStream(int parameterIndex, InputStream arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setBinaryStream(int, java.io.InputStream, int)
	 */
	public void setBinaryStream(int parameterIndex, InputStream arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setUnicodeStream(int, java.io.InputStream, int)
	 */
	public void setUnicodeStream(int parameterIndex, InputStream arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setCharacterStream(int, java.io.Reader, int)
	 */
	public void setCharacterStream(int parameterIndex, Reader arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setObject(int, java.lang.Object)
	 */
	public void setObject(int parameterIndex, Object val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val.toString();
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setObject(int, java.lang.Object, int)
	 */
	public void setObject(int parameterIndex, Object arg1, int arg2) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setObject(int, java.lang.Object, int, int)
	 */
	public void setObject(int parameterIndex, Object arg1, int arg2, int arg3) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setNull(int, int, java.lang.String)
	 */
	public void setNull(int parameterIndex, int val, String ignored) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		setNull(parameterIndex,val);
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setString(int, java.lang.String)
	 */
	public void setString(int parameterIndex, String val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val;
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setBigDecimal(int, java.math.BigDecimal)
	 */
	public void setBigDecimal(int parameterIndex, BigDecimal val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val.toString();
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setURL(int, java.net.URL)
	 */
	public void setURL(int parameterIndex, URL val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val.toString();
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setArray(int, java.sql.Array)
	 */
	public void setArray(int parameterIndex, Array val) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setBlob(int, java.sql.Blob)
	 */
	public void setBlob(int parameterIndex, Blob arg1) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setClob(int, java.sql.Clob)
	 */
	public void setClob(int parameterIndex, Clob arg1) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setDate(int, java.sql.Date)
	 */
	public void setDate(int parameterIndex, Date val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val.toString();
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#getParameterMetaData()
	 */
	public ParameterMetaData getParameterMetaData() throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: PreparedStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setRef(int, java.sql.Ref)
	 */
	public void setRef(int parameterIndex, Ref arg1) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#executeQuery()
	 */
	public ResultSet executeQuery() throws SQLException {
		return super.executeQuery(formatSQL());
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#getMetaData()
	 */
	public ResultSetMetaData getMetaData() throws SQLException {
		return getResultSet().getMetaData();
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setTime(int, java.sql.Time)
	 */
	public void setTime(int parameterIndex, Time val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val.toString();
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setTimestamp(int, java.sql.Timestamp)
	 */
	public void setTimestamp(int parameterIndex, Timestamp val) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		values[parameterIndex-1] = val.toString();
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setDate(int, java.sql.Date, java.util.Calendar)
	 */
	public void setDate(int parameterIndex, Date val, Calendar cal) throws SQLException {
		if(parameterIndex<1|parameterIndex>values.length)
			throw new SQLException("Set value "+parameterIndex+1+": out of range");
		cal.setTime(val);
		values[parameterIndex-1] = cal.toString();
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setTime(int, java.sql.Time, java.util.Calendar)
	 */
	public void setTime(int parameterIndex, Time val, Calendar cal) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

	/* (non-Javadoc)
	 * @see java.sql.PreparedStatement#setTimestamp(int, java.sql.Timestamp, java.util.Calendar)
	 */
	public void setTimestamp(int parameterIndex, Timestamp val, Calendar cal) throws SQLException {
		throw new SQLException("Not Implemented: PreparedStatement()");
		
	}

}
