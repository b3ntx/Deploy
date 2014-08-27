#ifndef __CONFIG__
#define __CONFIG__
#include "Includes.h"
namespace DeployDriver
{
	static const DWORD nSIZE = 255;
	static const DWORD nMAXSRCFILES = nSIZE / 4;
	typedef struct _RECORD {
		TCHAR sourceFiles[nMAXSRCFILES][nSIZE];
		TCHAR hwID[nSIZE];
		TCHAR targetPath[nSIZE];
	} RECORD, *PRECORD;

	typedef enum {
		singleFile, multFiles, directory
	} FILETYPE;

	class Config
	{
	public:
		Config(void)
		{
			//iicout << "Config Constructor running" << endl;
			this->hRec = NULL;
			this->sourceFileCount = 0;
			memset(&rec, 0, sizeof(RECORD));
			this->currentSource = 0;
			this->configPath = 0;
		}
		~Config()
		{
			//iicout << "Destructor running" << endl;

			//iicout << "deleting hRec" << endl;
			freeHandle(this->hRec);
			//iicout << "deleting configPath" << endl;
			freeMemA(this->configPath);
		}

		void ConfigSummary()
		{
			DWORD i = 0;
			LPCSTR file = 0;
			
			icout << "Deployment Summary" << endl;
			icout << "Configuration file: " << this->configPath << endl;
			for (i = 0, file = NextSourceFile(TRUE); file != NULL; file = NextSourceFile(), ++i)
				icout << "File #" << i << ": " << file << endl;
			icout << "Destination Folder: " << this->rec.targetPath << endl;
			icout << "Hardware ID: " << this->rec.hwID << endl;
			icout << "End Deployment Summary" << endl;
		}

		BOOL SetHardwareID()
		{
			BOOL result = FALSE;
			TCHAR buf[nSIZE]; memset(buf, 0, nSIZE);

			icout << "Enter the hardware ID (e.g. Root\\KMDFDriver)\n> ";
			cin >> buf;

			if (result = (strnlen(buf, nSIZE) > 0))
			{
				// Clear rec.hwID before saving the new value
				memset(this->rec.hwID, 0, nSIZE);
				strncpy(this->rec.hwID, buf, nSIZE);
				icout << "Setting the hardware ID to: " << this->rec.hwID << endl;
				WriteRecordToDisk(&this->hRec, &this->rec);
			}
			else ecout << "Error setting the hardware ID\n\tReceived input: " << buf << endl;

			return result;
		}

		BOOL Load(BOOL newConfigFile = FALSE)
		{
			DWORD openFlag = OPEN_EXISTING;
			BOOL initConfig = FALSE;

			// If we have a config path already, and we want to replace it (i.e. get
			//	a new one), then free the memory from the old path
			if (this->configPath != 0 && newConfigFile == TRUE)
			{
				freeMemA(this->configPath);
			}
			if (this->configPath == 0)
			{
				// Since we're getting a new configuration file, make sure the handle to the
				//	old file is closed, and the source file count is reset
				freeHandle(this->hRec);
				this->sourceFileCount = this->currentSource = 0;
				// Get the path to the new configuration file
				this->configPath = GetPathFromDialog("Open Configuration File", singleFile);
			}

			// DBG print out the path after we get it
			//iicout << "configPath = " << configPath << endl;

		GetConfigHandle:
			freeHandle(this->hRec);
			this->hRec = CreateFile(
				this->configPath,
				FILE_READ_ACCESS | FILE_WRITE_ACCESS,
				FILE_SHARE_READ,
				NULL,
				openFlag,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			if (this->hRec == INVALID_HANDLE_VALUE)
			{
				// If the file was not found, we are using a new (empty) configuration file
				if (!(initConfig = (BOOL)(GetLastError() == ERROR_FILE_NOT_FOUND)))
				{
					ecout << "Load(): Error creating/opening configuration file" << endl;
					return FALSE;
				}
				openFlag = OPEN_ALWAYS;
				this->hRec = NULL;
				goto GetConfigHandle;
			}
			
			if (initConfig == TRUE)
			{
				// Write an empty record into the new config file
				LONG filePtr = 0;
				RECORD lRec = { 0 };		// Local record zeroed out
				// Return FALSE if there is an error writing the blank record to disk
				if (WriteRecordToDisk(&this->hRec, &lRec) == FALSE)
					return FALSE;
				// Reset the file pointer to the beginning
				SetFilePointer(this->hRec, filePtr, &filePtr, FILE_BEGIN);
				// Now that the new config file is saved to disk and is the correct size,
				//	mark is as 'not' new
				initConfig = FALSE;
			}


			// We now have a populated configuration file and record.
			//	NOTE: If this is the first time creating a config file, the file
			//		and the record object will both be \0 throughout
			//		Else, the config file and record object should be populated with
			//		actual data

			return ReadRecordFromDisk(&this->hRec, &this->rec);
		}

		/* Allow the user to select multiple deployment files */
		/* If BOOL append = TRUE, then the files are simply added to the list of files already
		there.  Otherwise, the entire list is wiped out and replaced */
		BOOL SelectSourceFiles(BOOL append = FALSE)
		{
			LPSTR sourceFiles = 0;
			BOOL result = FALSE;
			// It is an error for the user to append files if there are no files to begin with
			//	If the user is appending files to the list, make sure the file count > 0
			if (append == TRUE && this->sourceFileCount == 0)
			{
				ecout << "Attempting to append source files while there are no current source files." << endl;
				ecout << "Please select the option corresponding to 'Select files to deploy' instead of the 'append' option" << endl;
				return FALSE;
			}
			// Get the source file string - each file delimited by a '\0', and the last file in the 
			//	string delimitted by '\0\0' - and validate/store the files
			if ((sourceFiles = GetPathFromDialog("Select Deployment Files", multFiles)) != NULL)
				// If ValidateAndCommitSourceFiles() succeeds, the sourceFileCount should be > 0
				result = (ValidateAndCommitSourceFiles(sourceFiles, append) && this->sourceFileCount > 0);
			if (result == FALSE)
				ecout << "SelectSourceFiles(): Could not get multiple source files" << endl;

			this->currentSource = 0;
			// Save the config file to disk
			WriteRecordToDisk(&this->hRec, &this->rec);
			freeMemA(sourceFiles);
			return result;
		}

		BOOL SelectDestinationPath()
		{
			LPSTR destPath = 0;
			BOOL validPath = FALSE;

			// Get the destination folder on the target system
			destPath = GetPathFromDialog("Select Deployment Destination Folder", directory);
			if (validPath = (destPath != NULL))
				// If the destination is valid (i.e. the user didn't cancel the operation),
				//	save the path to the record
				strncpy(this->rec.targetPath, destPath, nSIZE);
			else ecout << "SelectDestinationPath(): Could not receive a deployment destination folder" << endl;

			// Save the config file to disk
			WriteRecordToDisk(&this->hRec, &this->rec);
			freeMemA(destPath);
			return validPath;
		}

		BOOL IsRecordValid()
		{
			LPCSTR file = 0;
			BOOL valid = TRUE;

			// Make sure that each variable in the record object has a non-null string
			for (file = NextSourceFile(TRUE); file != NULL; file = NextSourceFile())
				valid &= (strnlen(file, nSIZE) > 0);

			// Test the destination path
			valid &= (strnlen(this->rec.targetPath, nSIZE) > 0);

			// Test the hardware ID
			valid &= (strnlen(this->rec.hwID, nSIZE) > 0);

			return valid;
		}

		/* NextSourceFile(BOOL) returns the next source file indicated by this->currentSource.
		If BOOL reset = TRUE, then this->currentSource will return the first source file in the record.
		Else NextSourceFile will return the next source in the record. */
		LPCSTR NextSourceFile(BOOL reset = FALSE)
		{
			if (reset == TRUE)
			{
				this->currentSource = 0;
				return this->rec.sourceFiles[this->currentSource++];
			}
			// reset == FALSE - get the next source file in the list
			else
				// Return the next file in the list, or return NULL if there is none
				return ((this->currentSource < this->sourceFileCount) ? this->rec.sourceFiles[this->currentSource++] : NULL);
		}

		LPCSTR GetDestinationPath() const
		{ return this->rec.targetPath; }

		LPCSTR GetHardwareID() const
		{ return this->rec.hwID; }

		const DWORD GetSourceFileCount() const
		{ return this->sourceFileCount; }

	private:
		/* When the GetOpenFileDialog returns multiple files, each file is delimited
		by a '\0' in the buffer with the *last* file delimited by '\0\0'.
		We want each of the null-terminated strings in a separate slot in rec.sourceFiles[n]. */
		BOOL ValidateAndCommitSourceFiles (LPSTR tokStr, BOOL append = FALSE)
		{
			LPSTR dir = tokStr,										// dir = base directory
				current = tokStr + 
				((strnlen(tokStr, nSIZE) + 1)*sizeof(TCHAR)),		// current = first file
				base = current;
			TCHAR delim = '\0';

			// If BOOL append = TRUE, append the new source files
			//	Otherwise, just erase all current files and start from 0
			if (append == FALSE)
				sourceFileCount = 0;

			current = strtok(base, &delim);
			// Clear out the source file array
			for (DWORD i = sourceFileCount; i < nMAXSRCFILES; ++i)
			{
				// Save some cpu cycles when dealing with small deployments
				//	Don't call memset if the first character (rec.sourceFiles[i][0]) is null
				//	This means that the entire string is null and memset will have no effect 
				if (this->rec.sourceFiles[i][0] != '\0')
					memset(this->rec.sourceFiles[i], 0, nSIZE);
				if (current != NULL)
				{
					// Save the dir+file into sourceFiles[currentSource]
					strncpy(this->rec.sourceFiles[sourceFileCount], dir, nSIZE);
					this->rec.sourceFiles[sourceFileCount][strnlen(this->rec.sourceFiles[sourceFileCount], nSIZE)] = '\\';
					strncat(this->rec.sourceFiles[sourceFileCount], current, nSIZE);
					icout << "File #" << sourceFileCount << ": " << this->rec.sourceFiles[sourceFileCount] << endl;
					
					current = strtok(
						(base += strnlen(current, nSIZE)*sizeof(TCHAR)+sizeof(TCHAR)), &delim);

					++sourceFileCount;
				}
			}

			return TRUE;
		}

		/* Read the record, pRec, from the *OPEN* handle, hFile */
		BOOL ReadRecordFromDisk (PHANDLE hFile, PRECORD pRec)
		{
			DWORD bytesRead = 0;
			LONG filePtr = 0;
			BOOL result = FALSE;
			LPCSTR file = 0;

			if (hFile != NULL && pRec != NULL &&
				ReadFile(*hFile, pRec, sizeof(RECORD), &bytesRead, NULL) &&
				bytesRead == sizeof(RECORD))		// Sanity check
			{
				DWORD tmp = 0;
				// On successful read, set all of the class members accordingly
				// Set the source file count to 99 (arbitrary) to avoid errors in NextSourceFile() 
				//	The count will be corrected after for loop
				this->sourceFileCount = 99;
				for (tmp = 0, file = NextSourceFile(TRUE);
					file != NULL && strnlen(file, nSIZE) != 0;
					tmp++, file = NextSourceFile()) { };
				// Set sourceFileCount to the real value
				this->sourceFileCount = tmp;

				result = TRUE;
			}
			else
			{
				ecout << "ReadRecordFromDisk(): ReadFile() failed or the number of bytes read"
					" does not match the size of the record\n\t# of bytes read: " <<
					bytesRead << "\n\tsizeof(RECORD): " << sizeof(RECORD) << endl;
				result = FALSE;
			}
			ResetFilePointerPosition(hFile);
			return result;
		}

		/* Write the record, pRec, to disk using the *OPEN* handle, hFile */
		BOOL WriteRecordToDisk (PHANDLE hFile, PRECORD pRec)
		{
			DWORD bytesWritten = 0;
			LONG filePtr = 0;
			BOOL result = FALSE;

			if (hFile != NULL && pRec != NULL &&
				WriteFile(*hFile, pRec, sizeof(RECORD), &bytesWritten, NULL) &&
				bytesWritten == sizeof(RECORD))		// Sanity check
				result = TRUE;
			else
			{
				ecout << "WriteRecordToDisk(): WriteFile() failed or the number of bytes written"
					" does not match the size of the record\n\t# of bytes written: " <<
					bytesWritten << "\n\tsizeof(RECORD): " << sizeof(RECORD) << endl;
				result = FALSE;
			}
			ResetFilePointerPosition(hFile);
			return result;
		}

		void ResetFilePointerPosition(PHANDLE hFile)
		{
			LARGE_INTEGER ptr = { 0 }, current = { 0 };

			// Get the current file pointer position
			SetFilePointerEx(*hFile, ptr, &current, FILE_CURRENT);
			if (current.QuadPart != 0)
			{
				// If the current file ptr is not at the beginning, move it to the beginning
				current.QuadPart = -current.QuadPart;
				SetFilePointerEx(*hFile, current, &ptr, FILE_CURRENT);
			}
		}

		/* LPCSTR title denotes the dialog window's title */
		/* If FILETYPE ftype = multFiles, multiply the buffer size by 2^4 */
		LPSTR GetPathFromDialog(LPCSTR title, FILETYPE ftype)
		{
			OPENFILENAME ofn = { 0 };
			DWORD size = ftype == multFiles ? (nSIZE << 4) : nSIZE;
			LPSTR pathStr = new TCHAR[size];
			memset(pathStr, 0, size);

			ofn.lpstrTitle = title;			// Title
			ofn.lpstrFile = pathStr;		// Buffer for selected file(s)
			ofn.nMaxFile = size;			// Size of above buffer
			ofn.lStructSize = sizeof(OPENFILENAME); // Have to set this for some reason

			switch (ftype)
			{
			case multFiles:
				ofn.Flags = OFN_ALLOWMULTISELECT;
			case singleFile:
				ofn.Flags |= OFN_EXPLORER | OFN_CREATEPROMPT;

				// Show the dialog and get the result
				if (!GetOpenFileName(&ofn))
				{
					ecout << "GetOpenFileName FAILED\n"
						<< "Disregaurd failure if you canceled the \"Select File(s)\" operation" << endl;
					freeMemA(pathStr);
				}
				// Fix the formatting error which occurs when the user wishes to select multiple files, but actually
				//	only selects on file.  The restulting format of this action causes the source file validation to break.
				//	If this condition is true, just alter the format of the single returned file so that it conforms to
				//	the format expected and described in the comments of the source file validation function.

				// The condition which needs to be fixed is when:
				//	ftype == multFiles
				//		AND
				//	pathStr has a file extension (this is true if ofn.nFileExtension > 0)
				//		Otherwise, if the user actually selected multiple files, pathStr would only contain the
				//			directory which the files are stored in.
				if (ftype == multFiles && ofn.nFileExtension > 0)
				{
					pathStr[ofn.nFileOffset - 1] = '\0';
				}
				break;
			case directory:
				ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR;

				if (GetSaveFileName(&ofn))
					/* SUCH BAD HACK INCOMING... */
					// Because Windows is impossibly difficult to simply get a directory, we are going to just ask for a 
					//	place to save a file, and then deploy all of the source files into the directory of the supplied
					//	file (without creating the supplied file).

					// So, at this point, the user has given us a file name (probably not even created yet).  We are going to
					//	strip the file name from 'pathStr' and save the resulting directory path into the record.

					// Suppose that the user supplied the following file path (now sitting in 'pathStr'):
					//	C:\Users\CoolUser1\Desktop\Deployment\trashfile.trash

					// Inserting a '\0' character at pathStr[ofn.nFileOffset] will effectively remove the file name from the
					//	path resulting in 'pathStr' = C:\Users\CoolUser1\Desktop\Deployment\ 
					pathStr[ofn.nFileOffset] = '\0';
				else
				{
					ecout << "GetSaveFileName FAILED\n"
						<< "Disregaurd failure if you canceled the \"Select File(s)\" operation" << endl;
					freeMemA(pathStr);
				}
				break;
			default:
				break;
			}

			return pathStr;
		}

		LPSTR configPath;
		RECORD rec;
		HANDLE hRec;
		DWORD currentSource;
		DWORD sourceFileCount;


	};
}

#endif