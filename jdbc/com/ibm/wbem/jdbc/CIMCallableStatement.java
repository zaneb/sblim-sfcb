/*
 * CIMCallableStatement.java
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
 * Description: Implementaion of the interface CallableStatement for the CIM-JDBC
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
import java.sql.CallableStatement;
import java.sql.Clob;
import java.sql.Date;
import java.sql.Ref;
import java.sql.SQLException;
import java.sql.Time;
import java.sql.Timestamp;
import java.util.Calendar;
import java.util.Map;

/*
 * Created on 16.02.2005
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 */

/**
 * @author bentele
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class CIMCallableStatement extends CIMPreparedStatement implements CallableStatement {
 
	/**
	 * @param s
	 * @param connection
	 * @param sql
	 * @throws SQLException
	 */
	public CIMCallableStatement(Socket s, CIMConnection connection, String sql) throws SQLException {
		super(s, connection, sql);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param s
	 * @param connection
	 * @param sql
	 * @throws SQLException
	 */
	

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#wasNull()
	 */
	public boolean wasNull() throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getByte(int)
	 */
	public byte getByte(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getDouble(int)
	 */
	public double getDouble(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getFloat(int)
	 */
	public float getFloat(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getInt(int)
	 */
	public int getInt(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getLong(int)
	 */
	public long getLong(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getShort(int)
	 */
	public short getShort(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getBoolean(int)
	 */
	public boolean getBoolean(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getBytes(int)
	 */
	public byte[] getBytes(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#registerOutParameter(int, int)
	 */
	public void registerOutParameter(int arg0, int arg1) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#registerOutParameter(int, int, int)
	 */
	public void registerOutParameter(int arg0, int arg1, int arg2) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getObject(int)
	 */
	public Object getObject(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getString(int)
	 */
	public String getString(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#registerOutParameter(int, int, java.lang.String)
	 */
	public void registerOutParameter(int arg0, int arg1, String arg2) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getByte(java.lang.String)
	 */
	public byte getByte(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getDouble(java.lang.String)
	 */
	public double getDouble(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getFloat(java.lang.String)
	 */
	public float getFloat(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getInt(java.lang.String)
	 */
	public int getInt(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getLong(java.lang.String)
	 */
	public long getLong(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getShort(java.lang.String)
	 */
	public short getShort(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getBoolean(java.lang.String)
	 */
	public boolean getBoolean(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getBytes(java.lang.String)
	 */
	public byte[] getBytes(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setByte(java.lang.String, byte)
	 */
	public void setByte(String arg0, byte arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setDouble(java.lang.String, double)
	 */
	public void setDouble(String arg0, double arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setFloat(java.lang.String, float)
	 */
	public void setFloat(String arg0, float arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#registerOutParameter(java.lang.String, int)
	 */
	public void registerOutParameter(String arg0, int arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setInt(java.lang.String, int)
	 */
	public void setInt(String arg0, int arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setNull(java.lang.String, int)
	 */
	public void setNull(String arg0, int arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#registerOutParameter(java.lang.String, int, int)
	 */
	public void registerOutParameter(String arg0, int arg1, int arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setLong(java.lang.String, long)
	 */
	public void setLong(String arg0, long arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setShort(java.lang.String, short)
	 */
	public void setShort(String arg0, short arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setBoolean(java.lang.String, boolean)
	 */
	public void setBoolean(String arg0, boolean arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setBytes(java.lang.String, byte[])
	 */
	public void setBytes(String arg0, byte[] arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getBigDecimal(int)
	 */
	public BigDecimal getBigDecimal(int arg0) throws SQLException {
		return new BigDecimal(111);
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getBigDecimal(int, int)
	 */
	public BigDecimal getBigDecimal(int arg0, int arg1) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getURL(int)
	 */
	public URL getURL(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getArray(int)
	 */
	public Array getArray(int arg0) throws SQLException {
		throw new SQLException("Not Implemented: CallableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getBlob(int)
	 */
	public Blob getBlob(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getClob(int)
	 */
	public Clob getClob(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getDate(int)
	 */
	public Date getDate(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getRef(int)
	 */
	public Ref getRef(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getTime(int)
	 */
	public Time getTime(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getTimestamp(int)
	 */
	public Timestamp getTimestamp(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setAsciiStream(java.lang.String, java.io.InputStream, int)
	 */
	public void setAsciiStream(String arg0, InputStream arg1, int arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setBinaryStream(java.lang.String, java.io.InputStream, int)
	 */
	public void setBinaryStream(String arg0, InputStream arg1, int arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setCharacterStream(java.lang.String, java.io.Reader, int)
	 */
	public void setCharacterStream(String arg0, Reader arg1, int arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getObject(java.lang.String)
	 */
	public Object getObject(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setObject(java.lang.String, java.lang.Object)
	 */
	public void setObject(String arg0, Object arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setObject(java.lang.String, java.lang.Object, int)
	 */
	public void setObject(String arg0, Object arg1, int arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setObject(java.lang.String, java.lang.Object, int, int)
	 */
	public void setObject(String arg0, Object arg1, int arg2, int arg3) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getObject(int, java.util.Map)
	 */
	public Object getObject(int arg0, Map arg1) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getString(java.lang.String)
	 */
	public String getString(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#registerOutParameter(java.lang.String, int, java.lang.String)
	 */
	public void registerOutParameter(String arg0, int arg1, String arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setNull(java.lang.String, int, java.lang.String)
	 */
	public void setNull(String arg0, int arg1, String arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setString(java.lang.String, java.lang.String)
	 */
	public void setString(String arg0, String arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getBigDecimal(java.lang.String)
	 */
	public BigDecimal getBigDecimal(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setBigDecimal(java.lang.String, java.math.BigDecimal)
	 */
	public void setBigDecimal(String arg0, BigDecimal arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getURL(java.lang.String)
	 */
	public URL getURL(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setURL(java.lang.String, java.net.URL)
	 */
	public void setURL(String arg0, URL arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getArray(java.lang.String)
	 */
	public Array getArray(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getBlob(java.lang.String)
	 */
	public Blob getBlob(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getClob(java.lang.String)
	 */
	public Clob getClob(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getDate(java.lang.String)
	 */
	public Date getDate(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setDate(java.lang.String, java.sql.Date)
	 */
	public void setDate(String arg0, Date arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getDate(int, java.util.Calendar)
	 */
	public Date getDate(int arg0, Calendar arg1) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getRef(java.lang.String)
	 */
	public Ref getRef(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getTime(java.lang.String)
	 */
	public Time getTime(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setTime(java.lang.String, java.sql.Time)
	 */
	public void setTime(String arg0, Time arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getTime(int, java.util.Calendar)
	 */
	public Time getTime(int arg0, Calendar arg1) throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getTimestamp(java.lang.String)
	 */
	public Timestamp getTimestamp(String arg0) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setTimestamp(java.lang.String, java.sql.Timestamp)
	 */
	public void setTimestamp(String arg0, Timestamp arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getTimestamp(int, java.util.Calendar)
	 */
	public Timestamp getTimestamp(int arg0, Calendar arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getObject(java.lang.String, java.util.Map)
	 */
	public Object getObject(String arg0, Map arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getDate(java.lang.String, java.util.Calendar)
	 */
	public Date getDate(String arg0, Calendar arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getTime(java.lang.String, java.util.Calendar)
	 */
	public Time getTime(String arg0, Calendar arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#getTimestamp(java.lang.String, java.util.Calendar)
	 */
	public Timestamp getTimestamp(String arg0, Calendar arg1) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setDate(java.lang.String, java.sql.Date, java.util.Calendar)
	 */
	public void setDate(String arg0, Date arg1, Calendar arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setTime(java.lang.String, java.sql.Time, java.util.Calendar)
	 */
	public void setTime(String arg0, Time arg1, Calendar arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}

	/* (non-Javadoc)
	 * @see java.sql.CallableStatement#setTimestamp(java.lang.String, java.sql.Timestamp, java.util.Calendar)
	 */
	public void setTimestamp(String arg0, Timestamp arg1, Calendar arg2) throws SQLException {
		// TODO Auto-generated method stub
		throw new SQLException("Not Implemented: callableStatement()");
	}



}
