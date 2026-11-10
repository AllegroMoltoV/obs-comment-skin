# 見た目の崩れの検査 (ドック)
# 使い方: pwsh dock-check.ps1 -ProcessId <OBS の PID>
# 区画をすべて開き、スクロールバーの値を変えながら、表示範囲に入った欄の位置と大きさを UI Automation で読む。
# スクロールバーとの重なり、ドックの右端からのはみ出し、欄どうしの重なり、狭すぎる欄を書き出す。
# 崩れが 1 件でもあれば終了コード 1 で終わる。検査の後、スクロールの位置を先頭に戻す
param([Parameter(Mandatory)] [int] $ProcessId)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$AE = [System.Windows.Automation.AutomationElement]
$Scope = [System.Windows.Automation.TreeScope]
$CT = [System.Windows.Automation.ControlType]

$main = $AE::RootElement.FindFirst($Scope::Children,
    (New-Object System.Windows.Automation.PropertyCondition($AE::ProcessIdProperty, $ProcessId)))
$dock = $main.FindFirst($Scope::Descendants,
    (New-Object System.Windows.Automation.PropertyCondition($AE::NameProperty, 'コメント欄の着せ替え / Comment Skin')))
if (-not $dock) { throw 'dock not found' }

# 区画の見出しは、チェックボックスとして見える開閉のボタン。閉じているものを開く
$sectionNames = @('メッセージ', '名前', 'アイコン', '時刻', '文字の縁取り', '背景', 'Super Chat とメンバーシップ', '表示する部分', 'アニメーション')
foreach ($name in $sectionNames) {
    $header = $dock.FindFirst($Scope::Descendants, (New-Object System.Windows.Automation.AndCondition(
        (New-Object System.Windows.Automation.PropertyCondition($AE::NameProperty, $name)),
        (New-Object System.Windows.Automation.PropertyCondition($AE::ControlTypeProperty, $CT::CheckBox)))))
    if (-not $header) { continue }
    $toggle = $header.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern)
    if ($toggle.Current.ToggleState -eq 'Off') { $toggle.Toggle() }
}
Start-Sleep -Milliseconds 500

$scrollBars = @($dock.FindAll($Scope::Descendants, (New-Object System.Windows.Automation.PropertyCondition($AE::ControlTypeProperty, $CT::ScrollBar))))
# ドックの縦のスクロールバーは、縦長のスクロールバーのうち最も背の高いもの。
# 生成した CSS の欄などにも縦のスクロールバーがあるため、高さで見分ける
$vbar = $scrollBars | Where-Object {
    $r = $_.Current.BoundingRectangle
    $_.Current.FrameworkId -ne 'Chrome' -and $r.Height -gt $r.Width -and
        $_.GetCurrentPattern([System.Windows.Automation.RangeValuePattern]::Pattern).Current.Maximum -gt 0
} | Sort-Object { $_.Current.BoundingRectangle.Height } -Descending | Select-Object -First 1

$dockRect = $dock.Current.BoundingRectangle
$types = @($CT::Button, $CT::ComboBox, $CT::Spinner, $CT::CheckBox, $CT::Text, $CT::Edit, $CT::Slider)
$issues = New-Object System.Collections.Generic.List[string]
$checked = @{}

function Get-Controls {
    $all = $dock.FindAll($Scope::Descendants, [System.Windows.Automation.Condition]::TrueCondition)
    foreach ($e in $all) {
        if ($types -notcontains $e.Current.ControlType) { continue }
        # 見本はブラウザーの中のページで、その文字の要素はドックの欄ではない
        if ($e.Current.FrameworkId -eq 'Chrome') { continue }
        $r = $e.Current.BoundingRectangle
        if ($r.IsEmpty -or $r.Width -le 0 -or $r.Height -le 0) { continue }
        # ドックの見出しとフローティングのボタンは OBS の部品のため除く
        if ($e.Current.Name -in @('コメント欄の着せ替え / Comment Skin', '閉じる', 'Float')) { continue }
        [pscustomobject]@{ Name = $e.Current.Name; Type = $e.Current.ControlType.ProgrammaticName; Rect = $r }
    }
}

function Test-Viewport {
    $view = if ($vbar) {
        $b = $vbar.Current.BoundingRectangle
        [System.Windows.Rect]::new($dockRect.Left, $b.Top, $b.Left - $dockRect.Left, $b.Height)
    } else { $dockRect }
    $controls = @(Get-Controls | Where-Object { $_.Rect.Top -ge $view.Top -and $_.Rect.Bottom -le $view.Bottom })
    foreach ($c in $controls) {
        $key = "$($c.Type)|$($c.Name)|$([int]$c.Rect.Left)"
        $checked[$key] = $true
        if ($c.Rect.Right -gt $view.Right + 1) {
            $issues.Add("beyond-right-edge|$($c.Type)|$($c.Name)|right=$([int]$c.Rect.Right) view=$([int]$view.Right)")
        }
        if ($c.Type -in @('ControlType.ComboBox', 'ControlType.Spinner', 'ControlType.Edit', 'ControlType.Slider') -and $c.Rect.Width -lt 40) {
            $issues.Add("too-narrow|$($c.Type)|$($c.Name)|width=$([int]$c.Rect.Width)")
        }
    }
    for ($i = 0; $i -lt $controls.Count; $i++) {
        for ($j = $i + 1; $j -lt $controls.Count; $j++) {
            $a = $controls[$i].Rect; $b = $controls[$j].Rect
            # 一方がもう一方を含むものは、部品とその中身として扱う
            if ($a.Contains($b) -or $b.Contains($a)) { continue }
            $w = [Math]::Min($a.Right, $b.Right) - [Math]::Max($a.Left, $b.Left)
            $h = [Math]::Min($a.Bottom, $b.Bottom) - [Math]::Max($a.Top, $b.Top)
            if ($w -gt 2 -and $h -gt 2) {
                $issues.Add("overlap|$($controls[$i].Name)|$($controls[$j].Name)|$([int]$w)x$([int]$h)")
            }
        }
    }
}

if ($vbar) {
    $range = $vbar.GetCurrentPattern([System.Windows.Automation.RangeValuePattern]::Pattern)
    $max = $range.Current.Maximum
    $step = [Math]::Max(20, [int]($vbar.Current.BoundingRectangle.Height * 0.8))
    for ($v = 0; $v -le $max + $step; $v += $step) {
        $range.SetValue([Math]::Min($v, $max))
        Start-Sleep -Milliseconds 200
        Test-Viewport
    }
    $range.SetValue(0)
} else {
    Test-Viewport
}

$unique = $issues | Sort-Object -Unique
[pscustomobject]@{
    dockWidth = [int]$dockRect.Width
    dockHeight = [int]$dockRect.Height
    scrollBar = [bool]$vbar
    checkedControls = $checked.Count
    issueCount = @($unique).Count
    issues = @($unique)
} | ConvertTo-Json -Depth 3
if (@($unique).Count -gt 0) { exit 1 }
