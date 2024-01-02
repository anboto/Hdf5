// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2023, the Anboto author and contributors
#ifndef _hdf5_h_
#define _hdf5_h_

#include <plugin/Hdf5/src/hdf5.h>
#include <Eigen/Eigen.h>

namespace Upp {

class Hid {
public:
	Hid() {}
	Hid(hid_t _id) : id(_id) {};
	virtual void Close() = 0;
	operator hid_t() const   {return id;};
	
protected:
	hid_t id = -1;	
};

class HidO : public Hid {
public:
	HidO() {}
	HidO(hid_t _id) : Hid(_id) {};
	~HidO() 			 {Close();}
	
	void Close() {
        if (id >= 0)
            H5Oclose(id);
        id = -1;
    }
    
	HidO& operator=(hid_t newId) {
        Close();
        id = newId;
        return *this;
    }
};

class HidS : public Hid {
public:
	HidS() {}
	HidS(hid_t _id) : Hid(_id) {};
	~HidS() 			 {Close();}
	
	void Close() {
        if (id >= 0)
            H5Sclose(id);
        id = -1;
    }
    
	HidS& operator=(hid_t newId) {
        Close();
        id = newId;
        return *this;
    }
};

class HidD : public Hid {
public:
	HidD() {}
	HidD(hid_t _id) : Hid(_id) {};
	~HidD() 			 {Close();}
	
	void Close() {
        if (id >= 0)
            H5Dclose(id);
        id = -1;
    }
	
	HidD& operator=(hid_t newId) {
        Close();
        id = newId;
        return *this;
    }
};

class Hdf5File {
public:
	Hdf5File()				{}
	Hdf5File(String file)	{Open(file);}
	~Hdf5File()				{Close();}
	
	bool Create(String file);		
	bool Open(String file, unsigned mode = H5F_ACC_RDWR);
	void Close();
	
	bool ChangeGroup(String group);
	Vector<String> ListGroup(bool groups, bool datasets);
	Vector<String> ListGroup()			{return ListGroup(true, true);}
	Vector<String> ListGroupGroups()	{return ListGroup(true, false);}
	Vector<String> ListGroupDatasets()	{return ListGroup(false, true);}
	void UpGroup();
	bool CreateGroup(String group);

	bool Exist(String name, bool isgroup);
	bool ExistDataset(String name)		{return Exist(name, false);}
	bool ExistGroup(String name)		{return Exist(name, true);}
	
	bool Delete(String name);
	
	void GetType(String name, H5T_class_t &type, Vector<hsize_t> &dims);
	H5T_class_t GetType(String name) {
		H5T_class_t type;
		Vector<hsize_t> dummy;
		GetType(name, type, dummy);
		return type;
	}
	
	int GetInt(String name);
	double GetDouble(String name);
	String GetString(String name);
	void GetDouble(String name, Eigen::VectorXd &data);
	void GetDouble(String name, Vector<double> &data);
	void GetDouble(String name, Eigen::MatrixXd &data);
	
	Hdf5File &Set(String name, int d);
	Hdf5File &Set(String name, double d);
	Hdf5File &Set(String name, String d);
	Hdf5File &Set(String name, Eigen::VectorXd &d);
	Hdf5File &Set(String name, Vector<double> &d);
	Hdf5File &Set(String name, Eigen::MatrixXd &d);
	
	Hdf5File &SetDescription(String description);
	Hdf5File &SetUnits(String units);
	
	String GetLastError();
	void SurpressErrorMsgs() 				{H5Eset_auto2(H5E_DEFAULT, NULL, NULL);}

private:
	hid_t file_id = -1;
	HidD dts_id;
	Vector<hid_t> group_ids;
	
	void GetData0(String name, HidO &obj_id, hid_t &datatype_id, hid_t &dspace, int &sz, Vector<hsize_t> &dims);
	static void SetAttributes0(hid_t dset_id, String attribute, String val);
    static void SetAttributes(hid_t dset_id, String description, String units);
};

}
	
#endif

