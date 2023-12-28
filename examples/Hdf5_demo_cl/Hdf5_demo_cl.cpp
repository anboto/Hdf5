// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2022, the Anboto author and contributors
#include <Core/Core.h>

using namespace Upp;

#include <Hdf5/hdf5.h>


void CreateDataset(String file) {
    hid_t   file_id, dataset_id, dataspace_id; /* identifiers */
    hsize_t dims[2];
    herr_t  status;

    /* Create a new file using default properties. */
    file_id = H5Fcreate(file, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /* Create the data space for the dataset. */
    dims[0]      = 4;
    dims[1]      = 6;
    dataspace_id = H5Screate_simple(2, dims, NULL);

    /* Create the dataset. */
    dataset_id =
        H5Dcreate2(file_id, "/dset", H5T_STD_I32BE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /* End access to the dataset and release resources used by it. */
    status = H5Dclose(dataset_id);

    /* Terminate access to the data space. */
    status = H5Sclose(dataspace_id);

    /* Close the file. */
    status = H5Fclose(file_id);
}

void Test_hdf5(String file) {
    hid_t  file_id, dataset_id; /* identifiers */
    herr_t status;
    int    i, j, dset_data[4][6];

    /* Initialize the dataset. */
    for (i = 0; i < 4; i++)
        for (j = 0; j < 6; j++)
            dset_data[i][j] = i * 6 + j + 1;

    /* Open an existing file. */
    file_id = H5Fopen(file, H5F_ACC_RDWR, H5P_DEFAULT);

    /* Open an existing dataset. */
    dataset_id = H5Dopen2(file_id, "/dset", H5P_DEFAULT);

    /* Write the dataset. */
    status = H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset_data);

    status = H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset_data);

    /* Close the dataset. */
    status = H5Dclose(dataset_id);

    /* Close the file. */
    status = H5Fclose(file_id);
}


void IterateDataset(hid_t group_id, String parent, int indentation) {
	herr_t status;
	
    // Iterate over objects in the group and identify datasets
    H5G_info_t group_info;
    H5Gget_info(group_id, &group_info);

	String sindentation(' ', indentation);
	
    for (hsize_t i = 0; i < group_info.nlinks; i++) {
        char obj_name[256];
        H5Lget_name_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, obj_name, sizeof(obj_name), H5P_DEFAULT);

        // Check if the object is a dataset
        if (H5Lexists(group_id, obj_name, H5P_DEFAULT) >= 0) {
            hid_t obj_id = H5Oopen(group_id, obj_name, H5P_DEFAULT);
            H5O_info2_t oinfo;
            if (H5Oget_info(obj_id, &oinfo, H5O_INFO_BASIC) >= 0) {
                String child = parent + "/" + obj_name;
            	if (oinfo.type == H5O_TYPE_GROUP) {
                	printf("%sGroup: %s\n", ~sindentation, ~child);
                	IterateDataset(obj_id, child, indentation+2);
            	} else if (oinfo.type == H5O_TYPE_DATASET) {
                	hid_t dspace = H5Dget_space(obj_id);
                	const int ndims = H5Sget_simple_extent_ndims(dspace);
                	Buffer<hsize_t> dims(ndims);
					H5Sget_simple_extent_dims(dspace, ~dims, NULL);
					String sdims;
					for (int id = 0; id < ndims; ++id) {
						if (id > 0)
							sdims += ",";	
						sdims += FormatInt(dims[id]);
					}
					if (ndims > 0)
						sdims = "[" + sdims + "]";
                	printf("%sDataset: %s%s", ~sindentation, ~child, ~sdims);
                	
                 	// Get the datatype of the dataset
				    hid_t datatype_id = H5Dget_type(obj_id);
				    if (datatype_id >= 0) {
					    // Determine the datatype class
					    H5T_class_t clss = H5Tget_class(datatype_id);
					    switch (clss) {
					   	case H5T_INTEGER:	printf("(int)");	break;
					   	case H5T_FLOAT:		printf("(float)");	break;
					    case H5T_STRING:	printf("(string)");	break;
					    default:			printf("(?)");
					    }
				    
					    hsize_t len = H5Dget_storage_size(obj_id);
					    for (int id = 0; id < ndims; ++id) 
							len /= dims[id];
						
					    printf(" %d ", len);
					    
					    if (ndims == 0 && clss == H5T_STRING) {
					        StringBuffer s(len);
					        if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, ~s) >= 0)
					            printf("'%s' ", ~s);
					    } else if (clss == H5T_FLOAT) {
					        if (ndims == 1) {
					            Eigen::VectorXd v(dims[0]);
					            if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, v.data()) >= 0) 
					            	Cout() << v;
					        } else if (ndims == 2) {
					         	Buffer<double> d(dims[0]*dims[1]);  
					         	if (H5Dread(obj_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, d) >= 0) {
					         		Eigen::MatrixXd m = Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(d, dims[0], dims[1]);
					         		Cout() << m;
					         	}
					        }   
					    }
				    }
    				printf("\n");
                	
            	} else if (oinfo.type == H5O_TYPE_NAMED_DATATYPE) 
                	printf("%sNamed data type: %s\n", ~sindentation, ~child);
            	else if (oinfo.type == H5O_TYPE_MAP) 
                	printf("%sMap: %s\n", ~sindentation, ~child);
            }
            H5Oclose(obj_id);
        }
    }
}
	
bool IterateDataset(String file) {
    // Open the HDF5 file
    hid_t file_id = H5Fopen(file, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) {
        fprintf(stderr, "Unable to open file.\n");
        return false;
    }

    // Open the root group (you can also open other groups if datasets are stored in subgroups)
    hid_t group_id = H5Gopen2(file_id, "/", H5P_DEFAULT);
    if (group_id < 0) {
        fprintf(stderr, "Unable to open root group.\n");
        H5Fclose(file_id);
        return false;
    }
	
	IterateDataset(group_id, "", 0);
	
    // Close the group and file
    H5Gclose(group_id);
    H5Fclose(file_id);

    return true;
}



CONSOLE_APP_MAIN
{
	String file = AppendFileName(GetDesktopFolder(), "dset.h5");
	CreateDataset(file);
	IterateDataset(AppendFileName(GetDesktopFolder(), "oswec.h5"));
	Test_hdf5(file);
	
	Cout() << "\nProgram ended";
	#ifdef flagDEBUG
	ReadStdIn();
	#endif
}
