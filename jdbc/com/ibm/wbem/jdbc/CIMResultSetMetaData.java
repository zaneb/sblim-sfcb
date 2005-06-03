/*
 * Created on 16.02.2005
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
package com.ibm.wbem.jdbc;

import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Types;

/**
 * @author seyrich
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class CIMResultSetMetaData implements ResultSetMetaData {

	private String[][] col;
	private ResultSetMetaData rsmd;
	

	/**
	 * @param prop
	 */
	public CIMResultSetMetaData(String prop) {
		
		String[] st = prop.split("::");
		
		
		col = new String[st.length][];
		for (int i = 0; i < col.length; i++) {
			col[i] = st[i].split(";");
		}
		
		//für getColumnDisplaySize()
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getColumnCount()
	 */
	public int getColumnCount() throws SQLException {
		return col.length;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getColumnDisplaySize(int)
	 */
	public int getColumnDisplaySize(int column) throws SQLException {
		
			//Es wird nach der Anzahl der ZEICHEN gefragt!
			int indx = col[column][2].indexOf("(")>0? col[column][2].indexOf("("):col[column][2].length();
			String s = col[column][2].toUpperCase().substring(0,indx);
			if(s.equals("BIGINT"))  return -1;//Nachschlage --> CIMOM 
			else if(s.equals("TIMESTAMP"))  return -1;//Nachschlage --> CIMOM   
			else if(s.equals("DATE"))  return -1;//Nachschlage --> CIMOM    
			else if(s.equals("VARCHAR")){
				return Integer.parseInt(col[column][2].substring(col[column][2].indexOf("(")+1,col[column][2].length()-1));
			}  
			else if(s.equals("CHAR")) return -1;//Nachschlage --> CIMOM  
			else if(s.equals("DECIMAL") 
					|| s.equals("NUMERIC") 
					|| s.equals("NUM") 
					|| s.equals("DEC")){ 
				//das +1 muss sein, wegen des "," in der Darstellung
				return Integer.parseInt(col[column][2].substring(col[column][2].indexOf("(")+1,col[column][2].indexOf(",")))+1; 
			}
			else if(s.equals("DOUBLE")) return -1;//Nachschlage --> CIMOM  
			else if(s.equals("REAL")) return -1;//Nachschlage --> CIMOM  
			else if(s.equals("INTEGER")) return -1;//Nachschlage --> CIMOM  
			else if(s.equals("INT")) return -1;//Nachschlage --> CIMOM  
			else if(s.equals("SMALLINT")) return -1;//Nachschlage --> CIMOM  
			else return 0;
				
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getColumnType(int)
	 */
	public int getColumnType(int column) throws SQLException {
		int indx = col[column][2].indexOf("(")>0? col[column][2].indexOf("("):col[column][2].length();
		String s = col[column][2].toUpperCase().substring(0,indx);
		if(s.equals("BIGINT")) return Types.BIGINT; 
		if(s.equals("TIMESTAMP")) return Types.TIMESTAMP; 
		if(s.equals("DATE")) return Types.DATE;  
		if(s.equals("VARCHAR")) return Types.VARCHAR;
		if(s.equals("CHAR")) return Types.CHAR; 
		if(s.equals("DECIMAL")) return Types.DECIMAL;
		if(s.equals("NUMERIC")) return Types.DECIMAL;
		if(s.equals("NUM")) return Types.DECIMAL;
		if(s.equals("DEC")) return Types.DECIMAL;
		if(s.equals("DOUBLE")) return Types.DOUBLE;
		if(s.equals("REAL")) return Types.REAL;
		if(s.equals("INTEGER")) return Types.INTEGER;
		if(s.equals("INT")) return Types.INTEGER;
		if(s.equals("SMALLINT")) return Types.SMALLINT;
		if(s.equals("BOOLEAN")) return Types.BOOLEAN;
		if(s.equals("REF")) return Types.REF;
		if(s.equals("OTHER")) return Types.OTHER;
		//System.out.println(s);
		throw new SQLException();
		
		
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getPrecision(int)
	 */
	public int getPrecision(int column) throws SQLException {
		if(getColumnType(column)==Types.DECIMAL)
			return Integer.parseInt(col[column][2].substring(col[column][2].indexOf("(")+1,col[column][2].indexOf(","))); 
		return 0;	
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getScale(int)
	 */
	public int getScale(int column) throws SQLException {
		if(getColumnType(column)==Types.DECIMAL)
			return Integer.parseInt(col[column][2].substring(col[column][2].indexOf(",")+1,col[column][2].indexOf(")"))); 
		return 0;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#isNullable(int)
	 */
	public int isNullable(int column) throws SQLException {
		// TODO Auto-generated method stub
		return Integer.parseInt(col[column][4]);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#isAutoIncrement(int)
	 */
	public boolean isAutoIncrement(int arg0) throws SQLException {
		return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#isCaseSensitive(int)
	 */
	public boolean isCaseSensitive(int column) throws SQLException {
		return col[column][5].equalsIgnoreCase("true");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#isCurrency(int)
	 */
	public boolean isCurrency(int arg0) throws SQLException {
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#isDefinitelyWritable(int)
	 */
	public boolean isDefinitelyWritable(int column) throws SQLException {
		return col[column][3].equalsIgnoreCase("true");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#isReadOnly(int)
	 */
	public boolean isReadOnly(int column) throws SQLException {
		return !isDefinitelyWritable(column);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#isSearchable(int)
	 */
	public boolean isSearchable(int arg0) throws SQLException {
		return true;
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#isSigned(int)
	 */
	public boolean isSigned(int arg0) throws SQLException {
		return true; //MIT CIMOM-DT ABGLEICHEN
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#isWritable(int)
	 */
	public boolean isWritable(int column) throws SQLException {
		return isDefinitelyWritable(column);
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getCatalogName(int)
	 */
	public String getCatalogName(int arg0) throws SQLException {
		throw new SQLException("Catalog not supported");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getColumnClassName(int)
	 */
	public String getColumnClassName(int column) throws SQLException {
//		Synchron zu CIMResultSetMetaData.getColumnClassName() halten
		//System.out.println(column);
		switch(getColumnType(column)){
			case Types.BIGINT: return "java.math.BigInteger";
			case Types.TIMESTAMP: return "java.sql.Timestamp";
			case Types.DATE: return "java.lang.Long";
			case Types.VARCHAR: return "java.lang.String";
			case Types.CHAR: return "java.lang.String";
			case Types.DECIMAL: return "java.math.BigDecimal";
			case Types.DOUBLE: return "java.lang.Double";
			case Types.REAL: return "java.lang.Float";
			case Types.INTEGER: return "java.lang.Integer";
			case Types.SMALLINT: return "java.lang.Integer";
			case Types.NULL: return "null";
			case Types.BOOLEAN: return "java.lang.Boolean";
			case Types.REF: return "java.lang.String";
			case Types.OTHER: return "java.lang.String";
			default: throw new SQLException();
		}
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getColumnLabel(int)
	 */
	public String getColumnLabel(int column) throws SQLException {
		return col[column][1];
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getColumnName(int)
	 */
	public String getColumnName(int column) throws SQLException {
		return col[column][1];
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getColumnTypeName(int)
	 */
	public String getColumnTypeName(int column) throws SQLException {
		return col[column][2];
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getSchemaName(int)
	 */
	public String getSchemaName(int column) throws SQLException {
		throw new SQLException("Schema not supported");
	}

	/* (non-Javadoc)
	 * @see java.sql.ResultSetMetaData#getTableName(int)
	 */
	public String getTableName(int column) throws SQLException {
		return col[column][0];
	}
}
