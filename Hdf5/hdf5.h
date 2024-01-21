// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2023, the Anboto author and contributors
#ifndef _hdf5_h_
#define _hdf5_h_

#include <plugin/Hdf5/src/hdf5.h>
#include <Eigen/Eigen.h>
#include <Eigen/MultiDimMatrixIndex.h>

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
	bool CreateGroup(String group, bool change = false);

	bool Exist(String name, bool isgroup);
	bool ExistDataset(String name)		{return Exist(name, false);}
	bool ExistGroup(String name)		{return Exist(name, true);}
	
	bool Delete(String name);
	
	void GetType(String name, H5T_class_t &type, Vector<int> &dims);
	
	H5T_class_t GetType(String name) {
		H5T_class_t type;
		Vector<int> dummy;
		GetType(name, type, dummy);
		return type;
	}
	
	int GetInt(String name);
	double GetDouble(String name);
	String GetString(String name);
	void GetDouble(String name, Eigen::VectorXd &data);
	void GetDouble(String name, Vector<double> &data);
	void GetDouble(String name, Eigen::MatrixXd &data);
	void GetDouble(String name, MultiDimMatrixRowMajor<double> &d);
	template <int Rank>
	void GetDouble(String name, Eigen::Tensor<double, Rank> &data) {
		int sz;
		HidO obj_id;
		hid_t datatype_id, dspace;
		Vector<int> dims;
		GetData0(name, obj_id, datatype_id, dspace, sz, dims);
	
		if (dims.size() != Rank)
			throw Exc(Format("Dimension different than %d", Rank));
	
		H5T_class_t clss = H5Tget_class(datatype_id);
		if (clss != H5T_FLOAT)
			throw Exc("Dataset is not double");
		
		Buffer<double> d_row(sz), d_col(sz);
		if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, d_row.Get()) < 0) 
			throw Exc("Impossible to read data");		

		RowMajorToColMajor(~d_row, ~d_col, dims);
		
		Eigen::array<Eigen::Index, Rank> dimensions;
		for (int i = 0; i < Rank; ++i)
			dimensions[i] = dims[i];

		data = Eigen::TensorMap<Eigen::Tensor<double, Rank>>(~d_col, dimensions);
	}
	
	Hdf5File &Set(String name, int d);
	Hdf5File &Set(String name, double d);
	Hdf5File &Set(String name, const char *d);
	Hdf5File &Set(String name, const Eigen::VectorXd &d);
	Hdf5File &Set(String name, const Vector<double> &d);
	Hdf5File &Set(String name, const Eigen::MatrixXd &d);
	Hdf5File &Set(String name, const MultiDimMatrixRowMajor<double> &d);
	template <int Rank>
	Hdf5File &Set(String name, const Eigen::Tensor<double, Rank> &d) {
		if (ExistDataset(name))
			Delete(name);
			
		Buffer<hsize_t> dims(Rank);
		Vector<int> dimensions(Rank);
		hsize_t sz = 1;
		for (int i = 0; i < Rank; ++i) 
			sz *= dims[i] = d.dimension(i);
		for (int i = 0; i < Rank; ++i) 
			dimensions[i] = int(d.dimension(i));
	    HidS dataspace_id = H5Screate_simple(Rank, dims, NULL);
	    if (dataspace_id < 0) 
	        throw Exc("Error creating dataspace");
	    
	    if ((dts_id = H5Dcreate2(Last(group_ids), name, H5T_NATIVE_DOUBLE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
	        throw Exc("Error creating dataset");
		
		Buffer<double> d_row(sz);
		ColMajorToRowMajor(d.data(), ~d_row, dimensions);
		
	    if (H5Dwrite(dts_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, ~d_row) < 0) 
	        throw Exc("Error writing data to dataset");
	    
	    return *this;		
	}
	
	Hdf5File &SetDescription(String description);
	Hdf5File &SetUnits(String units);
	
	String GetLastError();
	void SurpressErrorMsgs() 				{H5Eset_auto2(H5E_DEFAULT, NULL, NULL);}

private:
	hid_t file_id = -1;
	HidD dts_id;
	Vector<hid_t> group_ids;
	
	void GetData0(String name, HidO &obj_id, hid_t &datatype_id, hid_t &dspace, int &sz, Vector<int> &dims);
	static void SetAttributes0(hid_t dset_id, String attribute, String val);
    static void SetAttributes(hid_t dset_id, String description, String units);
};

}
	
#endif

