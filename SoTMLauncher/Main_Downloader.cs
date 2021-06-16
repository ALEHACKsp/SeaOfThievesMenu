using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.IO.Compression;
using System.Diagnostics;
using System.ComponentModel;

namespace SotMLauncher
{
    public partial class Main
    {
        class DownloadWorker_TaskInfo
        {
            public GithubWorker_TaskProgress_ReleaseInfo Release;

            public string SOTM_ReleaseFilePath = "";
            public string SOTM_DebugFilePath = "";

            public bool Success = false;

            public DownloadWorker_TaskInfo(GithubWorker_TaskProgress_ReleaseInfo release)
            {
                Release = release;
            }
        }
        // NOTE:
        // this class borrows githubClient from Main_Github
        // it's responsibilities are distinct enough that i decided to separate their logic, but the data needs to be shared

        bool IsDownloadingMenuUpdate = false;
        bool IsDownloadingLauncherUpdate = false;

        public bool IsDownloading { get { return IsDownloadingMenuUpdate || IsDownloadingLauncherUpdate; } }

        void DownloadAndInstallMenuFromRelease(GithubWorker_TaskProgress_ReleaseInfo release)
        {
            IsDownloadingMenuUpdate = true;
            while (DownloadWorker.IsBusy)
            {
                System.Windows.Forms.Application.DoEvents();
            }
            Logger.Log.Write("Beginning menu download", Logger.ELogType.Info, rtxtLog);
            DownloadWorker.RunWorkerAsync(new DownloadWorker_TaskInfo(release));
        }

        void DownloadAndInstallLauncherFromRelease(GithubWorker_TaskProgress_ReleaseInfo release)
        {
            IsDownloadingLauncherUpdate = true;
            if (File.Exists(Process.GetCurrentProcess().MainModule.FileName))
            {
                // move the current launcher files to its own 'old' directory, then pass some arguments to the new launcher exe to progress the update step.
                // WARNING:
                // begins the assumption that the launcher makes on startup to search for old installations
                Directory.CreateDirectory(LAUNCHER_DELETION_DIR);
                foreach (string oldLauncherFile in LAUNCHER_FILES)
                {
                    File.Move(oldLauncherFile, LAUNCHER_DELETION_DIR + "\\" + oldLauncherFile);
                    Logger.Log.Write("Moved '" + oldLauncherFile + "' to deletion directory in preparation for update", Logger.ELogType.Info, rtxtLog);
                }
            }
            else
            {
                Logger.Log.WriteError("Could not get existing launcher file, cannot update.", rtxtLog);
                return;
            }
            while (DownloadWorker.IsBusy)
            {
                System.Windows.Forms.Application.DoEvents();
            }
            Logger.Log.Write("Beginning launcher download", Logger.ELogType.Info, rtxtLog);
            DownloadWorker.RunWorkerAsync(new DownloadWorker_TaskInfo(release));
        }

        void FinishInstallingNewRelease(DownloadWorker_TaskInfo taskInfo)
        {
            // check if modDir + "Release\\" directory exists
            // delete all files within if it does, otherwise create the directory
            if (Directory.Exists(Main.MOD_RELEASE_FOLDER_NAME))
            {
                Logger.Log.Write("Clearing " + Main.MOD_RELEASE_FOLDER_NAME + " of existing files", Logger.ELogType.Info, rtxtLog);
                foreach (string filename in Directory.EnumerateFiles(Main.MOD_RELEASE_FOLDER_NAME))
                {
                    File.Delete(filename);
                }
            }
            else
            {
                Logger.Log.Write("Creating new " + Main.MOD_RELEASE_FOLDER_NAME + " directory", Logger.ELogType.Info, rtxtLog);
                Directory.CreateDirectory(Main.MOD_RELEASE_FOLDER_NAME);
            }
            // WARNING:
            // assumes the release asset is a zip file
            // unzip taskInfo.releaseFilePath to modDir + "Release\\" directory
            ZipFile.ExtractToDirectory(taskInfo.SOTM_ReleaseFilePath, Main.MOD_RELEASE_FOLDER_NAME);
            File.Delete(taskInfo.SOTM_ReleaseFilePath);
            Logger.Log.Write("Extracted release assets to " + Main.MOD_RELEASE_FOLDER_NAME, Logger.ELogType.Info, rtxtLog);

            // check if modDir + "Debug\\" directory exists
            // delete all files within if it does, otherwise create the directory
            if (Directory.Exists(Main.MOD_DEBUG_FOLDER_NAME))
            {
                Logger.Log.Write("Clearing " + Main.MOD_DEBUG_FOLDER_NAME + " of existing files", Logger.ELogType.Info, rtxtLog);
                foreach (string filename in Directory.EnumerateFiles(Main.MOD_DEBUG_FOLDER_NAME))
                {
                    File.Delete(filename);
                }
            }
            else
            {
                Logger.Log.Write("Creating new " + Main.MOD_DEBUG_FOLDER_NAME + " directory", Logger.ELogType.Info, rtxtLog);
                Directory.CreateDirectory(Main.MOD_DEBUG_FOLDER_NAME);
            }
            // WARNING:
            // assumes the release asset is a zip file
            // unzip taskInfo.debugFilePath to modDir + "Debug\\" directory
            ZipFile.ExtractToDirectory(taskInfo.SOTM_DebugFilePath, Main.MOD_DEBUG_FOLDER_NAME);
            File.Delete(taskInfo.SOTM_DebugFilePath);
            Logger.Log.Write("Extracted debug assets to " + Main.MOD_DEBUG_FOLDER_NAME, Logger.ELogType.Info, rtxtLog);
            Logger.Log.Write("Done installing " + taskInfo.Release.TagSemver.ToString(), Logger.ELogType.Notification, rtxtLog, true);
            IsDownloadingMenuUpdate = false;
            ActiveProfile.LocalTaggedSemVer = taskInfo.Release.TagSemver;
            if (taskInfo.Release.TagSemver.CompareByPrecedence(LatestMenuRelease.TagSemver) >= 0)
            {
                StatusLbl_SOTM.ForeColor = System.Drawing.Color.LimeGreen;
                StatusLbl_SOTM.Text = "SOTM: OK";
            }
            else
            {
                StatusLbl_SOTM.ForeColor = System.Drawing.Color.DarkOrange;
                StatusLbl_SOTM.Text = "SOTM: Update!";
            }
            // TO-DO:
            // if currently using proxy version, we should delete version.dll from gamedir and copy this new version over
        }

        private void DownloadWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            DownloadWorker_TaskInfo taskInfo = e.Argument as DownloadWorker_TaskInfo;
            try
            {
                Logger.Log.Write("Downloading '" + taskInfo.Release.ReleaseZip_DownloadURL + "'...", Logger.ELogType.Info, rtxtLog);
                var taskReleaseDownload = githubClient.Connection.Get<byte[]>(new Uri(taskInfo.Release.ReleaseZip_DownloadURL), new Dictionary<string, string>(), "application/octet-stream");
                byte[] releaseDownloadBytes = taskReleaseDownload.Result.Body;
                taskInfo.SOTM_ReleaseFilePath = Main.MOD_RELATIVE_PATH + "Release.zip";
                File.WriteAllBytes(taskInfo.SOTM_ReleaseFilePath, releaseDownloadBytes);
                Logger.Log.Write("Saved " + releaseDownloadBytes.Length.ToString() + " bytes to file '" + taskInfo.SOTM_ReleaseFilePath + "'", Logger.ELogType.Info, rtxtLog);

                Logger.Log.Write("Downloading '" + taskInfo.Release.DebugZip_DownloadURL + "'...", Logger.ELogType.Info, rtxtLog);
                var taskDebugDownload = githubClient.Connection.Get<byte[]>(new Uri(taskInfo.Release.DebugZip_DownloadURL), new Dictionary<string, string>(), "application/octet-stream");
                byte[] debugDownloadBytes = taskDebugDownload.Result.Body;
                taskInfo.SOTM_DebugFilePath = Main.MOD_RELATIVE_PATH + "Debug.zip";
                File.WriteAllBytes(taskInfo.SOTM_DebugFilePath, debugDownloadBytes);
                Logger.Log.Write("Saved " + debugDownloadBytes.Length.ToString() + " bytes to file '" + taskInfo.SOTM_DebugFilePath + "'", Logger.ELogType.Info, rtxtLog);
            }
            catch (Exception ex)
            {
                Logger.Log.WriteError("Exception occurred in DownloadWorker\n" + ex.Message + "\n" + ex.StackTrace, rtxtLog, true);
                taskInfo.Success = false;
                e.Result = taskInfo;
                return;
            }
            taskInfo.Success = true;
            e.Result = taskInfo;
        }

        private void DownloadWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            DownloadWorker_TaskInfo taskInfo = e.Result as DownloadWorker_TaskInfo;

            if (taskInfo.Success)
            {
                Logger.Log.Write("Downloaded new SOTM version (" + taskInfo.Release.TagSemver.ToString() + ")\n" + taskInfo.SOTM_ReleaseFilePath + "\n" + taskInfo.SOTM_DebugFilePath, Logger.ELogType.Notification, rtxtLog);
                FinishInstallingNewRelease(taskInfo);
            }
        }
    }
}
