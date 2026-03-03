#!/usr/bin/env pwsh
cd C:\Users\danwo\CodeProjects\MaximumTrainer_Archived

git add src/_subclassQWT/myqwtdial.cpp src/_subclassQWT/workoutplotlist.cpp src/_subclassQWT/mydialbox.cpp .github/workflows/build.yml

$commitMessage = @"
Fix Qt5/Qt6 compilation errors across 3 platforms

- myqwtdial.cpp: Replace setPenWidthF() with setPenWidth() - QwtRoundScaleDraw
  does not have a setPenWidthF member (fixes Linux build error)
- workoutplotlist.cpp: Add missing #include <QPen> and #include <qwt_text.h>
  to resolve incomplete type errors for QPen and QwtText (fixes Mac build error)
- mydialbox.cpp: Replace deprecated QColor::dark()/light() with darker()/lighter()
  as dark() and light() are removed in Qt6
- build.yml: Switch Windows SFML download from Invoke-WebRequest to curl.exe -L
  to reliably follow redirects and fix transient network failures

Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>
"@

git commit -m $commitMessage

git push origin copilot/review-qt6-compatibility

Write-Host "---COMMIT SHA---"
git log -1 --format=%H
Write-Host "---PUSH OUTPUT ABOVE---"
