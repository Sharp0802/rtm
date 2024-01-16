using RTM.Service;

namespace RTM.ViewModels;

public class MainWindowViewModel : ViewModelBase
{
    public SummaryViewModel SummaryViewModel { get; } = new();

    public MainWindowViewModel()
    {
        SummarizeService.Instance.ViewModel = SummaryViewModel;
    }
}