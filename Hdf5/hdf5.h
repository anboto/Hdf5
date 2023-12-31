// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2023, the Anboto author and contributors
#ifndef _hdf5_h_
#define _hdf5_h_

#include <plugin/Hdf5/src/hdf5.h>
#include <Eigen/Eigen.h>

namespace Upp {


class HidO {
public:
	HidO() {}
	HidO(hid_t loc_id, const char *name, hid_t lapl_id) {
		Open(loc_id, name, lapl_id);
	}
	HidO(hid_t _id)		{id = _id;}
	hid_t Open(hid_t loc_id, const char *name, hid_t lapl_id) {
		id = H5Oopen(loc_id, name, lapl_id);
		return id;
	}
	~HidO() {
		if (id >= 0)
			H5Oclose(id);
	}
	operator hid_t() const 			{return id;};
	
private:
	hid_t id = -1;
};

class HidS {
public:
	HidS(hid_t _id)		{id = _id;}
	~HidS() {
		if (id >= 0)
			H5Sclose(id);
	}
	operator hid_t() const 			{return id;};
	
private:
	hid_t id = -1;
};

class HidD {
public:
	HidD(hid_t _id)		{id = _id;}
	~HidD() {
		if (id >= 0)
			H5Dclose(id);
	}
	operator hid_t() const 			{return id;};
	
private:
	hid_t id = -1;
};

class Hdf5File {
public:
	Hdf5File()				{}
	Hdf5File(String file)	{Open(file);}
	~Hdf5File()				{Close();}
	
	bool Create(String file);		
	bool Open(String file);
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
	
	void Set(String name, int d);
	void Set(String name, double d);
	void Set(String name, String d);
	void Set(String name, Eigen::VectorXd &d);
	void Set(String name, Vector<double> &d);
	void Set(String name, Eigen::MatrixXd &d);
	
	void SurpressErrors() 				{H5Eset_auto2(H5E_DEFAULT, NULL, NULL);}

private:
	hid_t file_id = -1;
	Vector<hid_t> group_ids;
	
	void GetData0(String name, HidO &obj_id, hid_t &datatype_id, hid_t &dspace, int &sz, Vector<hsize_t> &dims);
};

}
	
#endif

