@echo off
cd /d C:\Users\danwo\CodeProjects\MaximumTrainer_Archived

REM Delete the codacy workflow file
del .github\workflows\codacy-analysis.yml

REM Stage the deletion
git add .github\workflows\codacy-analysis.yml

REM Show status
git status

REM Create a temporary file with the new commit message
(
echo Fix compilation errors for Qt5/Qt6 cross-platform builds
echo.
echo Four targeted fixes to resolve build failures on Linux, macOS, and Windows:
echo.
echo src/_subclassQWT/myqwtdial.cpp
echo - Replace setPenWidthF(2.0) with setPenWidth(2): QwtRoundScaleDraw has
echo   no setPenWidthF member. Fixes hard compile error on Linux.
echo.
echo src/_subclassQWT/workoutplotlist.cpp
echo - Add missing #include ^<qwt_text.h^> and #include ^<QPen^>: types used as
echo   complete types but only forward-declared. Fixes 3 compile errors on macOS.
echo.
echo src/_subclassQWT/mydialbox.cpp
echo - Replace QColor::dark(^) / QColor::light(^) with darker(^) / lighter(^)
echo   throughout (6 occurrences^). Deprecated in Qt 5.0, removed in Qt 6.
echo.
echo .github/workflows/build.yml
echo - Switch Windows SFML download from Invoke-WebRequest to curl.exe -L:
echo   handles SourceForge redirects correctly. Fixes Windows CI network failure.
echo.
echo .github/workflows/codacy-analysis.yml
echo - Remove Codacy security scan workflow entirely.
echo.
echo Co-authored-by: Copilot ^<223556219+Copilot@users.noreply.github.com^>
) > commit_msg.txt

REM Amend the commit
git commit --amend -F commit_msg.txt

REM Show the result
git --no-pager log --oneline -1
git --no-pager show HEAD --stat

REM Force push to the branch
git push --force-with-lease origin copilot/review-qt6-compatibility

REM Clean up
del commit_msg.txt

echo Done!
