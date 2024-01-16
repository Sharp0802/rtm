namespace RTM.ViewModels;

public class MainWindowViewModel : ViewModelBase
{
    public SummaryViewModel SummaryViewModel { get; } = new();
}