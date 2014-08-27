#ifndef __DEPLOY__
#define __DEPLOY__
#include "Includes.h"
#include "Config.h"

namespace DeployDriver
{

	class Deploy
	{
	private:
		Config conf;
		void _assnprintf(TCHAR *dest, DWORD size, TCHAR *fmt, int n, ...)
		{
			va_list va;
			va_start(va, n);
			vsnprintf(dest, size, fmt, va);
			va_end(va);
		}
		void FillCmdBuffer(TCHAR *buf, DWORD bufSize, int n, ...)
		{
			va_list va;
			va_start(va,n);
			vsnprintf(buf, bufSize, "%s \"%s\" %s\"", va);
			va_end(va);
		}

		void PrintMenu()
		{
			cout << "****************************************\n"
				<< "************** Main Menu ***************\n"
				<< "* 0 - Deploy\n"
				<< "* 1 - Load new configuration file\n"
				<< "* 2 - Select files to deploy\n"
				<< "* 3 - Select destination to deploy files\n"
				<< "* 4 - Enter the hardware ID of the driver\n"
				<< "* 5 - Print current configuration info\n"
				<< "* 6 - Append files to deployment list\n"
				<< "* 9 - Exit\n"
				<< "****************************************\n"
				<< "****************************************" << endl;
			cout << "> ";
		}

		void DeployBatchFile(TCHAR dir[], TCHAR inf[])
		{
			TCHAR installBatch[] = "_install.bat",
				removeBatch[] = "_remove.bat";
			const DWORD nBatchCmdLen = nSIZE << 2;
			DWORD nDestLen = 0;
			TCHAR *destPath = 0,
				cmdBuf[nBatchCmdLen],
				installCmd[] = "cmd.exe /K \"\"C:\\DriverTest\\devcon.exe\" -r install",
				removeCmd[] = "cmd.exe /K \"\"C:\\DriverTest\\devcon.exe\" -r remove";
			HANDLE hBatch = NULL;
			DWORD bWritten = 0;

			// Make destPath equal the absolute path of the install batch file
			nDestLen = strnlen(dir, nSIZE) + strnlen(installBatch, nSIZE) + 1;
			destPath = new TCHAR[nDestLen];
			strncpy(destPath, dir, nDestLen); 
			strncat(destPath, installBatch, nDestLen - strnlen(dir, nSIZE));

			icout << "*Install* batch file at: " << destPath << endl;

			hBatch = CreateFile(destPath,
				GENERIC_ALL,
				NULL,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_SYSTEM,
				NULL);

			if (hBatch == INVALID_HANDLE_VALUE)
			{
				ecout << "Error creating the **install** batch file" << endl;
				ErrorExit("DeployBatchFile()");
			}


			// Create the install script
			memset(cmdBuf, 0, nBatchCmdLen);
			_snprintf(cmdBuf, nBatchCmdLen, "%s \"%s\" %s\"", installCmd, inf, conf.GetHardwareID());
			if (WriteFile(hBatch, cmdBuf, strnlen(cmdBuf, nBatchCmdLen), &bWritten, NULL))
			{
				icout << "Batch file created on the target machine" << endl;
				freeHandle(hBatch); freeMemA(destPath);

				// Now create the remove script

				// Make destPath equal the absolute path of the install batch file
				nDestLen = strnlen(dir, nSIZE) + strnlen(removeBatch, nSIZE) + 1;
				destPath = new TCHAR[nDestLen];
				strncpy(destPath, dir, nDestLen);
				strncat(destPath, removeBatch, nDestLen - strnlen(dir, nSIZE));

				icout << "*Remove* batch file at: " << destPath << endl;

				hBatch = CreateFile(destPath,
					GENERIC_ALL,
					NULL,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_SYSTEM,
					NULL);

				if (hBatch == INVALID_HANDLE_VALUE)
				{
					ecout << "Error creating the **remove** batch file" << endl;
					ErrorExit("DeployBatchFile()");
				}
				bWritten = 0;
				// Create the un-install script
				memset(cmdBuf, 0, nBatchCmdLen);
				_snprintf(cmdBuf, nBatchCmdLen, "%s \"%s\"\"", removeCmd, conf.GetHardwareID());
				if (!WriteFile(hBatch, cmdBuf, strnlen(cmdBuf, nBatchCmdLen), &bWritten, NULL))
					ErrorExit("Writing un-install batch file..");
			}
			else ErrorExit("Writing install batch file..");

			freeHandle(hBatch);
			freeMemA(destPath);
		}

	public:
		Deploy()
		{
			//iicout << "Deploy Constructor is running" << endl;
			// Since this is the first time the Config class is active, prompt the user
			//	for a brand-new config file, or for the user to select a config file from disk
			//	NOTE: This is the same actions we take to reload a configuration file
			ReloadConfig();
		}

		BOOL ReloadConfig()
		{
			BOOL result = conf.Load(TRUE);
			if (result == FALSE)
				ecout << "ReloadConfig(): Could not load configuration file" << endl;
			return result;
		}

		BOOL DeployDriver()
		{
			BOOL result = FALSE, infFound = FALSE;
			LPCSTR file = 0;
			TCHAR destDir[nSIZE],
				filename[nSIZE],
				destFilenames[nMAXSRCFILES][nSIZE],
				infFilename[nSIZE];
			DWORD count = 0;
			// Load the current record from disk, verify it, then deploy it.
			if (conf.Load() && conf.IsRecordValid())
			{
				memset(destDir, 0, nSIZE); memset(infFilename, 0, nSIZE);
				strncpy(destDir, conf.GetDestinationPath(), nSIZE);
				
				for (count = 0, file = conf.NextSourceFile(TRUE); file != NULL; count++, file = conf.NextSourceFile())
				{
					// Clear the filename buffer to avoid any leftover characters from the last iteration
					memset(filename, 0, nSIZE); 
					// Copy the destination directory (including the trailing null-characters)
					memcpy(destFilenames[count], destDir, nSIZE);

					// Get a local copy of the source filename string (with full path)
					strncpy(filename, file, nSIZE);
					if (infFound == FALSE && strcmp(PathFindExtension(filename), ".inf") == 0)
					{
						infFound = TRUE;
						iicout << "INF found: " << filename << endl;
					}
					// Remove the source path from the string (i.e. get the filename w/extension)
					PathStripPath(filename);
					// Create the destination string (with full path)
					strncat(destFilenames[count], filename, nSIZE-1);
					if (infFound == TRUE && infFilename[0] == '\0')
						strncpy(infFilename, destFilenames[count], nSIZE);
					icout << "Copying file #" << count << ": " << filename << endl;
					if (CopyFile(file, destFilenames[count], FALSE) == 0)
						ErrorExit(_T("DeployDriver()"));
				}

				icout << "\nSuccessfully copied " << count << " files.\nCreating batch file.." << endl;
				DeployBatchFile(destDir, infFilename);
				icout << "Batch files created and copied successfully" << endl;

				result = TRUE;
			}
			else
			{
				// Record is not valid -- abort
				ecout << "Record is invalid\n\tPrint a configuration summary to see if any important options are missing" << endl;
				result = FALSE;
			}
			return result;
		}
		void Menu()
		{
			int sel = -1;
			BOOL wait = TRUE;

			while (wait)
			{
				system("CLS");

				PrintMenu();
				cin >> sel;
				//iicout << "sel = " << sel << endl;

				switch (sel)
				{
				case 0:
					if (DeployDriver() == FALSE)
						mbok("Error deploying driver...\nSee output log");
					break;
				case 1:
					if (ReloadConfig() == FALSE)
						mbok("Error loading configuration file...\nSee output log");
					break;
				case 2:
					if (conf.SelectSourceFiles() == FALSE)
						icout << "No files were selected" << endl;
					break;
				case 3:
					if (conf.SelectDestinationPath() == FALSE)
						icout << "Destination path not set" << endl;
					break;
				case 4:
					if (conf.SetHardwareID() == FALSE)
						icout << "Hardware ID not saved" << endl;
					break;
				case 5:
					conf.ConfigSummary();
					break;
				case 6:
					// Append files to source list
					if (conf.SelectSourceFiles(TRUE) == FALSE)
						icout << "No files were appended to the sources" << endl;
					break;
				case 9:
					wait = FALSE;
					break;
				default:
					ecout << "Incorrect input: " << sel << 
						"\n\tSelect from one of the menu options" << endl;
					break;
				}
				system("pause");
			}
		}

	};

}
#endif
