import java.sql.*;
import java.util.Enumeration;

/*
 * Created on 18.02.2005
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 */

/**
 * @author seyrich
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class TestClass {

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
//	Linux_ComputerSystem %% dyn-9-152-143-58.boeblingen.de.ibm.com %% Linux_OperatingSystem %% dyn-9-152-143-58.boeblingen.de.ibm.com %% Linux_UnixProcess %% 5850 %%Linux (Unix) Process %% This class represents instances of currently running programms. %% NULL %% 5830 %% pts/3 %% su %% /bin %% 24 %% 0 %% 500 %% 5830 %% 0 %% 6 %% 0 %% 0 %% 20050404065827.000000+120 %% su %% su %% 2 %% NULL %% 2 %% 
//	Linux_ComputerSystem", "dyn-9-152-143-58.boeblingen.de.ibm.com",  Linux_OperatingSystem"  "dyn-9-152-143-58.boeblingen.de.ibm.com"  "Linux_UnixProcess" ="5850"   
//	"Linux_ComputerSystem","dyn-9-152-143-58.boeblingen.de.ibm.com", "Linux_OperatingSystem", ,dyn-9-152-143-58.boeblingen.de.ibm.com", "Linux_UnixProcess", "5850","Linux (Unix) Process",="This class represents instances of currently running programms.", "NULL",="5830", "pts/3", "su", "/bin",  24,   0,   500,   5830,   0,   6,  =0,  =0,   20050404065827.000000+120,  "su", "su", =2,  "NULL",  2,  =2

	public static void main(String[] args) {
		ResultSet rs;
		Connection con = null;
		Statement stmt = null;
		try {
			Enumeration e = DriverManager.getDrivers();
			System.out.println("Installierte Driver:");
			while(e.hasMoreElements()){
				System.out.println("\t"+(e.nextElement()).toString());
			}
			con = DriverManager.getConnection("127.0.0.1:5980");
			
			//PreparedStatement pstmt = con.prepareStatement("Select Caption, UserModeTime  from  Linux_UnixProcess where UserModeTime not between ? And ?");
			//pstmt.setInt(1,13);
			//pstmt.setInt(2,4564);
			//rs = pstmt.executeQuery();
			
			stmt = con.createStatement(); 
			DatabaseMetaData dbmd = con.getMetaData();
			ResultSetMetaData rsms;
			int col,jj;
			
			//rs = dbmd.getTables(null,null,"CIM",null);
			//rs = dbmd.getSuperTables(null,null,"CIM");
		//	rs = dbmd.getPrimaryKeys(null,null,"Linux_CSBaseBoard");
		//	rs = dbmd.getPrimaryKeys(null,null,"Linux_CSBaseBoard");
			rs = dbmd.getColumns(null,null,"Linux","Capt");
			rsms = rs.getMetaData();
			col = rsms.getColumnCount();
			for (int i = 0; i < col; i++) {
				
				System.out.println("classname: "+rsms.getColumnClassName(i));
				System.out.println("displaysize: "+rsms.getColumnDisplaySize(i));
				System.out.println("columnlable: "+rsms.getColumnLabel(i));
				System.out.println("columnnane: "+rsms.getColumnName(i) +" "+i);
				System.out.println("type: "+rsms.getColumnType(i));
				System.out.println("typename: "+rsms.getColumnTypeName(i));
				System.out.println("precision: "+rsms.getPrecision(i));
				System.out.println("scale: "+rsms.getScale(i));
				System.out.println("TableName: "+rsms.getTableName(i));
				System.out.println("nullable: "+rsms.isNullable(i));
				System.out.println("readOnly: "+rsms.isReadOnly(i));
				System.out.println("Signed: "+rsms.isSigned(i));
				System.out.println("Writable: "+rsms.isWritable(i));
				
			}jj=0;
			System.out.println(col);
			while (rs.next()) {
				for (int i = 0; i < col; i++) {
					System.out.print(rs.getString(i+1)+" %% ");
				}
				System.out.println("");
			}
			System.out.println("Es tut 1");
		//	rs.close();//System.out.println("A");
		
			
			//rs = stmt.executeQuery("Select \n Caption, UserModeTime, a.WorkingSetSize, MaxRealStack, CIM_LogicalElement.ElementName, a.ElementName \n from Linux_UnixProcess as a, CIM_LogicalElement as b");
			// rs = stmt.executeQuery("Select Caption, UserModeTime, a.WorkingSetSize, MaxRealStack, CIM_LogicalElement.ElementName, a.ElementName  from  Linux_UnixProcess as a join CIM_LogicalElement as b on j=k");
			// rs = stmt.executeQuery("Select Caption, UserModeTime  from  Linux_UnixProcess");
		//	rs = stmt.executeQuery("Select distinct Caption   from  Linux_UnixProcess");
		//		rs = stmt.executeQuery("Select ElementName  from  CIM_LogicalElement");
		//	 rs = stmt.executeQuery("Select distinct *  from  Linux_UnixProcess fetch first 5 rows only");
			 //rs = stmt.executeQuery("Select *  from  Linux_UnixProcess union select Caption, UserModeTime from Linux_UnixProcess");
			//rs = stmt.executeQuery("Select *  from  Linux_UnixProcess union select Caption, UserModeTime from Linux_UnixProcess");
			//rs = stmt.executeQuery("(Select Caption, UserModeTime  from  Linux_UnixProcess union select Caption, UserModeTime from Linux_UnixProcess) union select Caption, UserModeTime from Linux_UnixProcess");
			// rs = stmt.executeQuery("Select Caption, UserModeTime  from  Linux_UnixProcess fetch first 5 rows only unionall select Caption, UserModeTime from Linux_UnixProcess fetch first 5 rows only");
			//rs = stmt.executeQuery("(Select Caption, UserModeTime  from  Linux_UnixProcess union select Caption, UserModeTime from Linux_UnixProcess) union (Select Caption, UserModeTime  from  Linux_UnixProcess union select Caption, UserModeTime from Linux_UnixProcess)");
		//	rs = stmt.executeQuery("(Select Caption, UserModeTime  from  Linux_UnixProcess order by 2 union select Caption, UserModeTime from Linux_UnixProcess) union (Select Caption, UserModeTime  from  Linux_UnixProcess union select Caption, UserModeTime from Linux_UnixProcess) order by UserModeTime desc fetch first 5 rows only");
			//rs = stmt.executeQuery("(Select Caption, UserModeTime  from  Linux_UnixProcess order by UserModeTime desc fetch first 3 rows only union select Caption, UserModeTime from Linux_UnixProcess order by UserModeTime asc fetch first 5 rows only)order by UserModeTime desc fetch first 5 rows only");
//			rs = stmt.executeQuery("(Select Caption, UserModeTime  from  Linux_UnixProcess order by UserModeTime desc fetch first 3 rows only union select Caption, UserModeTime from Linux_UnixProcess order by UserModeTime asc fetch first 5 rows only)order by UserModeTime desc");
			
//			rs = stmt.executeQuery("(Select Caption, UserModeTime  from  Linux_UnixProcess)");
			//rs = stmt.executeQuery("Select Caption, UserModeTime  from  Linux_UnixProcess fetch first 9 rows only exceptall select Caption, UserModeTime from Linux_UnixProcess fetch first 5 rows only");
			//rs = stmt.executeQuery("Select Caption, UserModeTime  from  Linux_UnixProcess fetch first 5 rows only intersect select Caption, UserModeTime from Linux_UnixProcess fetch first 5 rows only");
			//rs = stmt.executeQuery("Select Caption, UserModeTime  from  Linux_UnixProcess fetch first 5 rows only intersect select Caption, UserModeTime from Linux_UnixProcess order by Caption fetch first 5 rows only");
			//rs = stmt.executeQuery("Select all *  from  Linux_UnixProcess fetch first 5 rows only union select all * from Linux_UnixProcess order by Caption,UserModeTime fetch first 5 rows only");
			//rs = stmt.executeQuery("Select ParentProcessID,UserModeTime  from  Linux_UnixProcess fetch first 5 rows only union select ParentProcessID,UserModeTime from Linux_UnixProcess order by ParentProcessID,UserModeTime fetch first 5 rows only order by ParentProcessID");
			//System.out.println("Select Caption, UserModeTime  from  Linux_UnixProcess where UserModeTime!='asdads' AND Caption>\'hallo\'");
			
			//rs = stmt.executeQuery("Select Caption, UserModeTime  from  Linux_UnixProcess where UserModeTime==5 AND Caption>\"hallo\" or UserModeTime between 5 and 6");	
		//	rs = stmt.executeQuery("Select Caption, UserModeTime  from  Linux_UnixProcess where UserModeTime>100 OR UserModeTime=40 order by UserModeTime");// AND Caption>\"hallo\" or UserModeTime between 5 and 6");	
			//rs = stmt.executeQuery("Select Caption, UserModeTime as a  from  Linux_UnixProcess where a>100 OR UserModeTime=40 order by UserModeTime");// AND Caption>\"hallo\" or UserModeTime between 5 and 6");	
			//rs = stmt.executeQuery("Select Caption, t1.UserModeTime  from  Linux_UnixProcess as t1 where t1.UserModeTime>100 OR UserModeTime=40 order by UserModeTime");// AND Caption>\"hallo\" or UserModeTime between 5 and 6");	
		//	rs = stmt.executeQuery("Select Caption, UserModeTime  from  Linux_UnixProcess where UserModeTime not between 40 And 100 order by UserModeTime");// AND Caption>\"hallo\" or UserModeTime between 5 and 6");	
			//rs = stmt.executeQuery("Select Caption, UserModeTime  from  Linux_UnixProcess where UserModeTime not between 40 And 100 order by UserModeTime");// AND Caption>\"hallo\" or UserModeTime between 5 and 6");	
			//rs = stmt.executeQuery("Select Caption, UserModeTime  from  Linux_UnixProcess where UserModeTime not between 40 And 100 order by UserModeTime");// AND Caption>\"hallo\" or UserModeTime between 5 and 6");	
			
			// tut nicht rs = stmt.executeQuery("Select Caption, t1.UserModeTime t2.UserModeTime  from  Linux_UnixProcess as t1,Linux_UnixProcess as t2 where t1.UserModeTime=t2.UserModeTime and t1.UserModeTime=40 order by UserModeTime");// AND Caption>\"hallo\" or UserModeTime between 5 and 6");	
			//rs = stmt.executeQuery("Select *  from  Linux_UnixProcess ,Linux_UnixProcess  where t1.UserModeTime=t2.UserModeTime and t1.UserModeTime=40 order by UserModeTime");// AND Caption>\"hallo\" or UserModeTime between 5 and 6");	
			//rs = stmt.executeQuery("Select *  from  Linux_UnixProcess fetch first 5 rows only");	
			//rs = stmt.executeQuery("Select Caption, UserModeTime  from  Linux_UnixProcess,CIM_LogicalElement where CIM_LogicalElement.Caption=Linux_UnixProcess.Caption ");// AND Caption>\"hallo\" or UserModeTime between 5 and 6");	
			
			
		//	rs = stmt.executeQuery("Drop Table test6");	
		//	rs = stmt.executeQuery("Select * from tall");	
			//rs = stmt.executeQuery("update Linux_UnixProcess set UserModeTime=UserModeTime +-5, Caption=\"99\" where UserModeTime not between 40 And 100 AND Caption>\"hallo\"");	
		//	rs = stmt.executeQuery("update test_a set charlie=18");	
		//	rs = stmt.executeQuery("update test6 set s1=18");	
		//	rs = stmt.executeQuery("insert into  test_a (hugo,charlie) values (11,1),(12,1),(13,1)");	
		//	rs = stmt.executeQuery("insert into  test6 (s2,s1) values (11,1),(12,1),(13,1)");	
			//rs = stmt.executeQuery("insert into  Linux_UnixProcess (UserModeTime,Caption) select UserModeTime,Caption from Linux_UnixProcess");	
			//rs = stmt.executeQuery("insert into  Linux_UnixProcess (UserModeTime,Caption) select * from Linux_UnixProcess");	
			//rs = stmt.executeQuery("insert into  Linux_UnixProcess (UserModeTime,Caption) select Caption,UserModeTime from Linux_UnixProcess");	
			//rs = stmt.executeQuery("insert into  Linux_UnixProcess (UserModeTime,Caption) select UserModeTime,Caption from Linux_UnixProcess");	
			//rs = stmt.executeQuery("insert into  Linux_UnixProcess (UserModeTime,Caption) select distinct UserModeTime,Caption from Linux_UnixProcess");	
			//rs = stmt.executeQuery("delete from  tall");	
			//rs = stmt.executeQuery("call Linux_UnixProcess.hall9o0(a1,a2,a3)");	
		//	rs = stmt.executeQuery("create table test15 (spaa2 varchar(1) , spbb varchar(1) KEY)");	
		//	rs = stmt.executeQuery("create table test17 (spaa2 varchar(12) , spbb varchar(12) KEY)");
		//rs = stmt.executeQuery("create table test6 (s1 int , s2 real KEY)");
		//		rs = stmt.executeQuery("select * from test13");	
		//	rs = stmt.executeQuery("insert into  test_a (charlie, hugo) values  (20,30)");	
		//	rs = stmt.executeQuery("insert into  test16 (spbb,spaa2) values  (112,245), (113,245), (114,245)");	
		//	rs = stmt.executeQuery("insert into  test17 (spbb,spaa2) values  (\"a\",\"b\"), (\"c\",\"d\"), (\"e\",\"f\")");	
		//	rs = stmt.executeQuery("insert into  test (spbb,spaa2) values  (3,2), (4,2.321), (2,7.65)");	
		//	rs = stmt.executeQuery("delete from test_a  where hugo=3");
		//	rs = stmt.executeQuery("delete from test6  where s2=11");
		//	rs = stmt.executeQuery("delete from test6 ");
		//	rs = stmt.executeQuery("select * from  test6");	
		//	rs = stmt.executeQuery("delete  from  test20 ");	
		//	rs = stmt.executeQuery("delete  from  test18 where spbb=2");	
			//	rs = stmt.executeQuery("create table tabhh like CIM_LogicalElement)");	
			rs = stmt.executeQuery("call test6.sdf ()");	
		
			
			SQLWarning sw = stmt.getWarnings();
			System.out.println("SQLState: "+sw.getSQLState()+"\nSQLWarningText: "+sw.getMessage());
			if(sw.getSQLState().equals("00000")){
				rsms = rs.getMetaData();
				col = rsms.getColumnCount();
				for (int i = 0; i < col; i++) {
					
					System.out.println("classname: "+rsms.getColumnClassName(i));
					System.out.println("displaysize: "+rsms.getColumnDisplaySize(i));
					System.out.println("columnlable: "+rsms.getColumnLabel(i));
					System.out.println("columnnane: "+rsms.getColumnName(i) +" "+i);
					System.out.println("type: "+rsms.getColumnType(i));
					System.out.println("typename: "+rsms.getColumnTypeName(i));
					System.out.println("precision: "+rsms.getPrecision(i));
					System.out.println("scale: "+rsms.getScale(i));
					System.out.println("TableName: "+rsms.getTableName(i));
					System.out.println("nullable: "+rsms.isNullable(i));
					System.out.println("readOnly: "+rsms.isReadOnly(i));
					System.out.println("Signed: "+rsms.isSigned(i));
					System.out.println("Writable: "+rsms.isWritable(i));
					
				} jj=0;
				System.out.println(col);
				while (rs.next()) {
					for (int i = 0; i < col; i++) {
						System.out.print(rs.getString(i+1)+" %% ");
					}
					System.out.println("");
				}
				System.out.println("Es tut 3");
				rs.close();//System.out.println("A");
			}
			stmt.close();//System.out.println("B");
			con.close();//System.out.println("C");
		} catch (Exception e) {
			System.out.println("Fehler");
			e.printStackTrace();
			try {
				stmt.close();//System.out.println("B");
				con.close();//System.out.println("C");
			} catch (Exception sss) {
				// TODO: handle exception
		}
			// TODO: handle exception
		}
			
		/*String s ="TestTable::Column01;varchar(123456789);true;1;false::Column02;DOUBLE;false;0;false::Column03;DECIMAL(3,2);true;1;false"; 
		String[] ss = s.split("::");
		for (int i = 0; i < ss.length; i++) {
			System.out.println(ss[i]);
			String[] sss = ss[i].split(";");
			for (int j = 0; j < sss.length; j++) {
				System.out.println(sss[j]);
			}
		}*/
	}
}
