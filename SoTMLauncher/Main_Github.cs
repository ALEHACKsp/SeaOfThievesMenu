using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ComponentModel;
using Octokit;
using Semver;

namespace SotMLauncher
{
    public partial class Main
    {
        class GithubWorker_TaskInfo
        {
            public string FetchRepositoryName;
            public IReadOnlyList<Release> Releases;

            public GithubWorker_TaskInfo(string _fetchRepositoryName)
            {
                FetchRepositoryName = _fetchRepositoryName;
            }
        }

        class GithubWorker_TaskProgress_ReleaseInfo
        {
            public string FetchRepositoryName = "";
            public SemVersion TagSemver = null;
            public string ReleaseZip_DownloadURL = "";
            public string DebugZip_DownloadURL = "";
            public string LauncherExe_DownloadURL = "";

            public GithubWorker_TaskProgress_ReleaseInfo() { }

            public GithubWorker_TaskProgress_ReleaseInfo(string fetchRepositoryName, SemVersion tagSemver, string releaseDownloadURL, string debugDownloadURL, string launcherDownloadURL)
            {
                FetchRepositoryName = fetchRepositoryName;
                TagSemver = tagSemver;
                ReleaseZip_DownloadURL = releaseDownloadURL;
                DebugZip_DownloadURL = debugDownloadURL;
                LauncherExe_DownloadURL = launcherDownloadURL;
            }

            // probably not necessary but i don't care enough to find out
            public GithubWorker_TaskProgress_ReleaseInfo(GithubWorker_TaskProgress_ReleaseInfo other)
            {
                FetchRepositoryName = other.FetchRepositoryName;
                TagSemver = other.TagSemver;
                ReleaseZip_DownloadURL = other.ReleaseZip_DownloadURL;
                DebugZip_DownloadURL = other.DebugZip_DownloadURL;
                LauncherExe_DownloadURL = other.LauncherExe_DownloadURL;
            }

            public override string ToString()
            {
                return TagSemver.ToString();
            }
        }

        public const string GITHUB_OWNER = "BitCrackers";
        public const string GITHUB_SOTM_REPOSITORY = "SoTMenu";
        public const string GITHUB_PRODUCT_HEADER = "BitCrackers-SoTMenu-Launcher";

        public GitHubClient githubClient;

        bool IsFetching = false;

        List<GithubWorker_TaskProgress_ReleaseInfo> AllRepoReleases = new List<GithubWorker_TaskProgress_ReleaseInfo>();
        GithubWorker_TaskProgress_ReleaseInfo LatestLauncherRelease = null;
        GithubWorker_TaskProgress_ReleaseInfo LatestMenuRelease = null;
        bool SOTMUpdateAvailable = false;



        private void InitializeGithubClient()
        {
            githubClient = new GitHubClient(new ProductHeaderValue(GITHUB_PRODUCT_HEADER, LAUNCHER_VERSION));
            Logger.Log.Write("Initialized GithubClient", Logger.ELogType.Info, rtxtLog);
            FetchAllReleases();
        }

        void FetchAllReleases()
        {
            // spin while a previous fetch is in progress
            // this shouldn't ever happen, but just in case
            while (GithubWorker.IsBusy)
            {
                System.Windows.Forms.Application.DoEvents();
            }
            // we start by fetching menu releases, and when that's done we'll fetch launcher releases (done in the GithubWorker_RunWorkerCompleted event)
            GithubWorker.RunWorkerAsync(new GithubWorker_TaskInfo(GITHUB_SOTM_REPOSITORY));
        }


        private void GithubWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            GithubWorker_TaskInfo taskInfo = e.Argument as GithubWorker_TaskInfo;
            SOTMUpdateAvailable = false;
            // NOTE:
            // this call will be invoked on the main thread (this is why SafeCallDelegate exists)
            Logger.Log.Write("Github Worker now fetching '" + taskInfo.FetchRepositoryName + "' releases", Logger.ELogType.Info, rtxtLog);

            // this is an async call but we're already on a separate thread (background worker) so we can just block on it
            Task<IReadOnlyList<Release>> releasesTask = githubClient.Repository.Release.GetAll(GITHUB_OWNER, taskInfo.FetchRepositoryName, ApiOptions.None);
            taskInfo.Releases = releasesTask.Result;

            int numReleasesProcessed = 0;
            GithubWorker_TaskProgress_ReleaseInfo curReleaseInfo = new GithubWorker_TaskProgress_ReleaseInfo();
            curReleaseInfo.FetchRepositoryName = taskInfo.FetchRepositoryName;
            foreach (Release release in taskInfo.Releases)
            {
                numReleasesProcessed++;
                Logger.Log.Write("Saw release: " + release.TagName + "\n" + release.HtmlUrl, Logger.ELogType.Info, rtxtLog);
                if (release.Assets.Count == 0)
                {
                    Logger.Log.Write("Skipping release with zero assets", Logger.ELogType.Info, rtxtLog);
                    continue;
                }
                // clear old releaseInfo
                curReleaseInfo.TagSemver = null;
                curReleaseInfo.ReleaseZip_DownloadURL = "";
                curReleaseInfo.DebugZip_DownloadURL = "";
                curReleaseInfo.LauncherExe_DownloadURL = "";

                // parse semver out of tag, skipping the 'v' at the start.
                // WARNING:
                // expects tags to be named "v<semver>" where <semver> follows official semver 2.0 guidelines
                SemVersion releaseSemver;
                bool hasGoodParse = false;
                if (release.TagName.StartsWith("v"))
                    hasGoodParse = SemVersion.TryParse(release.TagName.Substring(1), out releaseSemver);
                else
                    hasGoodParse = SemVersion.TryParse(release.TagName, out releaseSemver);
                if (!hasGoodParse)
                {
                    Logger.Log.WriteError("Could not parse semver out of release tag (" + release.TagName + ")[" + release.HtmlUrl + "]", rtxtLog);
                    continue;
                }
                curReleaseInfo.TagSemver = releaseSemver;

                // process assets: save asset URLs and preliminary check for launcher version being greater than our current version
                foreach (ReleaseAsset asset in release.Assets)
                {
                    Logger.Log.Write("--Saw asset: " + asset.Name + "\n" + asset.BrowserDownloadUrl, Logger.ELogType.Info, rtxtLog);
                    // WARNING:
                    // makes assumptions about asset names
                    if (asset.Name.StartsWith("SOTM-launcher"))
                        curReleaseInfo.LauncherExe_DownloadURL = asset.Url;
                    else if (asset.Name.StartsWith("Release"))
                        curReleaseInfo.ReleaseZip_DownloadURL = asset.Url;
                    else if (asset.Name.StartsWith("Debug"))
                        curReleaseInfo.DebugZip_DownloadURL = asset.Url;
                }
                GithubWorker.ReportProgress((int)(((float)numReleasesProcessed / (float)taskInfo.Releases.Count) * 100f), new GithubWorker_TaskProgress_ReleaseInfo(curReleaseInfo));
            }
            e.Result = taskInfo;
        }

        private void GithubWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            GithubWorker_TaskProgress_ReleaseInfo releaseInfo = e.UserState as GithubWorker_TaskProgress_ReleaseInfo;
            if (releaseInfo.FetchRepositoryName == GITHUB_SOTM_REPOSITORY)
            {
                // add all SOTM releases to the drop-down box (we allow the user to select even outdated versions)
                Logger.Log.Write("Adding release '" + releaseInfo.TagSemver.ToString() + "' to combobox and AllReleases list", Logger.ELogType.Info, rtxtLog);
                cboxTaggedVersion.Items.Add(releaseInfo);
                AllRepoReleases.Add(releaseInfo);

                // check if the release is newer than what is installed locally
                if (releaseInfo.TagSemver.CompareByPrecedence(ActiveProfile.LocalTaggedSemVer) > 0)
                {
                    SOTMUpdateAvailable = true;
                }

                // keep track of the absolute newest SOTM version
                if (LatestMenuRelease == null)
                {
                    LatestMenuRelease = releaseInfo;
                }
                else if (releaseInfo.TagSemver.CompareByPrecedence(LatestMenuRelease.TagSemver) > 0)
                {
                    LatestMenuRelease = releaseInfo;
                }
            }
        }

        private void GithubWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            GithubWorker_TaskInfo taskInfo = e.Result as GithubWorker_TaskInfo;

            // a fetch task was complete, display to the user if any newer versions are available
            // (the actual processing is done in the GithubWorker_ProgressChanged event)
            Logger.Log.Write("GithubWorker finished fetching " + taskInfo.Releases.Count.ToString() + " releases from '" + taskInfo.FetchRepositoryName + "'", Logger.ELogType.Info, rtxtLog);
            if (taskInfo.FetchRepositoryName == GITHUB_SOTM_REPOSITORY)
            {
                if (SOTMUpdateAvailable)
                {
                    // notify user that they can select a newer build of SOTM, but don't force it (they may be using an older build or a pre-release build on purpose)
                    Logger.Log.Write("A newer version of SOTM is available. Select it in the combobox to auto-download.", Logger.ELogType.Notification, rtxtLog);
                    StatusLbl_SOTM.ForeColor = System.Drawing.Color.DarkOrange;
                    StatusLbl_SOTM.Text = "SOTM: Update!";
                }
                else
                {
                    StatusLbl_SOTM.ForeColor = System.Drawing.Color.LimeGreen;
                    StatusLbl_SOTM.Text = "SOTM: OK";
                }
            }
        }
    }
}
