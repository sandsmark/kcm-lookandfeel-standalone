/* This file is part of the KDE Project
   Copyright (c) 2014 Marco Martin <mart@kde.org>
   Copyright (c) 2014 Vishesh Handa <me@vhanda.in>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kcm.h"
#include "../krdb/krdb.h"
#include "../input/xcursor/xcursortheme.h"
#include "config-kcm.h"
#include <klauncher_iface.h>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KSharedConfig>
#include <QDebug>
#include <QStandardPaths>
#include <QProcess>
#include <QQuickWidget>
#include <KGlobalSettings>

#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QtQml>
#include <QQmlEngine>
#include <QQmlContext>
#include <QStandardItemModel>
#include <QX11Info>

#include <KLocalizedString>
#include <Plasma/Package>
#include <Plasma/PluginLoader>

#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>

#ifdef HAVE_XFIXES
#  include <X11/extensions/Xfixes.h>
#endif

K_PLUGIN_FACTORY(KCMLookandFeelFactory, registerPlugin<KCMLookandFeel>();)

KCMLookandFeel::KCMLookandFeel(QWidget* parent, const QVariantList& args)
    : KCModule(parent, args)
    , m_config("kdeglobals")
    , m_configGroup(m_config.group("KDE"))
    , m_applyColors(true)
    , m_applyWidgetStyle(true)
    , m_applyIcons(true)
    , m_applyPlasmaTheme(true)
    , m_applyCursors(true)
{
    qmlRegisterType<QStandardItemModel>();
    KAboutData* about = new KAboutData("kcm_lookandfeel", i18n("Configure Splash screen details"),
                                       "0.1", QString(), KAboutLicense::LGPL);
    about->addAuthor(i18n("Marco Martin"), QString(), "mart@kde.org");
    setAboutData(about);
    setButtons(Help | Apply | Default);

    m_model = new QStandardItemModel(this);
    QHash<int, QByteArray> roles = m_model->roleNames();
    roles[PluginNameRole] = "pluginName";
    roles[ScreenhotRole] = "screenshot";
    roles[HasSplashRole] = "hasSplash";
    roles[HasLockScreenRole] = "hasLockScreen";
    roles[HasRunCommandRole] = "hasRunCommand";
    roles[HasLogoutRole] = "hasLogout";

    roles[HasColorsRole] = "hasColors";
    roles[HasWidgetStyleRole] = "hasWidgetStyle";
    roles[HasIconsRole] = "hasIcons";
    roles[HasPlasmaThemeRole] = "hasPlasmaTheme";
    roles[HasCursorsRole] = "hasCursors";
    m_model->setItemRoleNames(roles);
    QVBoxLayout* layout = new QVBoxLayout(this);

    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/Generic");
    package.setDefaultPackageRoot("plasma/kcms");
    package.setPath("kcm_lookandfeel");
    m_quickWidget->rootContext()->setContextProperty("kcm", this);
    m_quickWidget->setSource(QUrl::fromLocalFile(package.filePath("mainscript")));
    setMinimumHeight(m_quickWidget->initialSize().height());

    layout->addWidget(m_quickWidget);
}

QStandardItemModel *KCMLookandFeel::lookAndFeelModel()
{
    return m_model;
}

QString KCMLookandFeel::selectedPlugin() const
{
    return m_selectedPlugin;
}

void KCMLookandFeel::setSelectedPlugin(const QString &plugin)
{
    if (m_selectedPlugin == plugin) {
        return;
    }

    m_selectedPlugin = plugin;
    emit selectedPluginChanged();
    changed();
}

void KCMLookandFeel::load()
{
    setSelectedPlugin(m_access.metadata().pluginName());

    m_model->clear();

    const QList<Plasma::Package> pkgs = LookAndFeelAccess::availablePackages();
    for (const Plasma::Package &pkg : pkgs) {
        QStandardItem* row = new QStandardItem(pkg.metadata().name());
        row->setData(pkg.metadata().pluginName(), PluginNameRole);
        row->setData(pkg.filePath("previews", "preview.png"), ScreenhotRole);

        //What the package provides
        row->setData(!pkg.filePath("splashmainscript").isEmpty(), HasSplashRole);
        row->setData(!pkg.filePath("lockscreenmainscript").isEmpty(), HasLockScreenRole);
        row->setData(!pkg.filePath("runcommandmainscript").isEmpty(), HasRunCommandRole);
        row->setData(!pkg.filePath("logoutmainscript").isEmpty(), HasLogoutRole);

        if (!pkg.filePath("defaults").isEmpty()) {
            KSharedConfigPtr conf = KSharedConfig::openConfig(pkg.filePath("defaults"));
            KConfigGroup cg(conf, "kdeglobals");
            cg = KConfigGroup(&cg, "KDE");
            bool hasColors = !cg.readEntry("ColorScheme", QString()).isEmpty();
            row->setData(hasColors, HasColorsRole);
            if (!hasColors) {
                hasColors = !pkg.filePath("colors").isEmpty();
            }
            row->setData(!cg.readEntry("widgetStyle", QString()).isEmpty(), HasWidgetStyleRole);
            cg = KConfigGroup(conf, "kdeglobals");
            cg = KConfigGroup(&cg, "Icons");
            row->setData(!cg.readEntry("Theme", QString()).isEmpty(), HasIconsRole);

            cg = KConfigGroup(conf, "kdeglobals");
            cg = KConfigGroup(&cg, "Theme");
            row->setData(!cg.readEntry("name", QString()).isEmpty(), HasPlasmaThemeRole);

            cg = KConfigGroup(conf, "kcminputrc");
            cg = KConfigGroup(&cg, "Mouse");
            row->setData(!cg.readEntry("cursorTheme", QString()).isEmpty(), HasCursorsRole);
        }

        m_model->appendRow(row);
    }
}


void KCMLookandFeel::save()
{
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/LookAndFeel");
    package.setPath(m_selectedPlugin);

    if (!package.isValid()) {
        return;
    }

    m_configGroup.writeEntry("LookAndFeelPackage", m_selectedPlugin);

    if (!package.filePath("defaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(package.filePath("defaults"));
        KConfigGroup cg(conf, "kdeglobals");
        cg = KConfigGroup(&cg, "KDE");
        if (m_applyWidgetStyle) {
            setWidgetStyle(cg.readEntry("widgetStyle", QString()));
        }

        if (m_applyColors) {
            QString colorsFile = package.filePath("colors");
            QString colorScheme = cg.readEntry("ColorScheme", QString());
            if (!colorsFile.isEmpty()) {
                if (!colorScheme.isEmpty()) {
                    setColors(colorScheme, colorsFile);
                } else {
                    setColors(package.metadata().name(), colorsFile);
                }
            } else if (!colorScheme.isEmpty()) {
                colorScheme.remove('\''); // So Foo's does not become FooS
                QRegExp fixer("[\\W,.-]+(.?)");
                int offset;
                while ((offset = fixer.indexIn(colorScheme)) >= 0)
                    colorScheme.replace(offset, fixer.matchedLength(), fixer.cap(1).toUpper());
                colorScheme.replace(0, 1, colorScheme.at(0).toUpper());
                QString src = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "color-schemes/" +  colorScheme + ".colors");
                setColors(colorScheme, src);
            }
        }

        if (m_applyIcons) {
            cg = KConfigGroup(conf, "kdeglobals");
            cg = KConfigGroup(&cg, "Icons");
            setIcons(cg.readEntry("Theme", QString()));
        }

        if (m_applyPlasmaTheme) {
            cg = KConfigGroup(conf, "plasmarc");
            cg = KConfigGroup(&cg, "Theme");
            setPlasmaTheme(cg.readEntry("name", QString()));
        }

        if (m_applyCursors) {
            cg = KConfigGroup(conf, "kcminputrc");
            cg = KConfigGroup(&cg, "Mouse");
            setCursorTheme(cg.readEntry("cursorTheme", QString()));
        }
    }

    m_configGroup.sync();
    runRdb(KRdbExportQtColors | KRdbExportGtkTheme | KRdbExportColors);
}

void KCMLookandFeel::defaults()
{
    setSelectedPlugin(m_access.metadata().pluginName());
}

void KCMLookandFeel::setWidgetStyle(const QString &style)
{
    if (style.isEmpty()) {
        return;
    }

    m_configGroup.writeEntry("widgetStyle", style);
    m_configGroup.sync();
    KGlobalSettings::self()->emitChange(KGlobalSettings::StyleChanged);
}

void KCMLookandFeel::setColors(const QString &scheme, const QString &colorFile)
{
    if (scheme.isEmpty() || colorFile.isEmpty()) {
        return;
    }

    m_configGroup.writeEntry("ColorScheme", scheme);

    KSharedConfigPtr conf = KSharedConfig::openConfig(colorFile);
    foreach (const QString &grp, conf->groupList()) {
      KConfigGroup cg(conf, grp);
      KConfigGroup cg2(&m_config, grp);
      cg.copyTo(&cg2);
  }
}

void KCMLookandFeel::setIcons(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    KConfigGroup cg(&m_config, "Icons");
    cg.writeEntry("Theme", theme);
}

void KCMLookandFeel::setPlasmaTheme(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    KConfig config("plasmarc");
    KConfigGroup cg(&config, "Theme");
    cg.writeEntry("name", theme);
    cg.sync();
}

void KCMLookandFeel::setCursorTheme(const QString themeName)
{
    //TODO: use pieces of cursor kcm when moved to plasma-desktop
    if (themeName.isEmpty()) {
        return;
    }

    KConfig config("kcminputrc");
    KConfigGroup cg(&config, "Mouse");
    cg.writeEntry("cursorTheme", themeName);
    cg.sync();

    // Require the Xcursor version that shipped with X11R6.9 or greater, since
    // in previous versions the Xfixes code wasn't enabled due to a bug in the
    // build system (freedesktop bug #975).
#if HAVE_XFIXES && XFIXES_MAJOR >= 2 && XCURSOR_LIB_VERSION >= 10105
    QDir themeDir = cursorThemeDir(themeName, 0);
qWarning()<<"FFFFFFFFFFFF"<<themeDir;
    if (!themeDir.exists()) {
        return;
    }

    XCursorTheme theme(themeDir);

    if (!CursorTheme::haveXfixes()) {
        return;
    }

    // Set up the proper launch environment for newly started apps
    OrgKdeKLauncherInterface klauncher(QStringLiteral("org.kde.klauncher5"),
                                       QStringLiteral("/KLauncher"),
                                       QDBusConnection::sessionBus());
    klauncher.setLaunchEnv(QStringLiteral("XCURSOR_THEME"), themeName);

    // Update the Xcursor X resources
    runRdb(0);

    // Notify all applications that the cursor theme has changed
    KGlobalSettings::self()->emitChange(KGlobalSettings::CursorChanged);

    // Reload the standard cursors
    QStringList names;

    // Qt cursors
    names << "left_ptr"       << "up_arrow"      << "cross"      << "wait"
          << "left_ptr_watch" << "ibeam"         << "size_ver"   << "size_hor"
          << "size_bdiag"     << "size_fdiag"    << "size_all"   << "split_v"
          << "split_h"        << "pointing_hand" << "openhand"
          << "closedhand"     << "forbidden"     << "whats_this" << "copy" << "move" << "link";

    // X core cursors
    names << "X_cursor"            << "right_ptr"           << "hand1"
          << "hand2"               << "watch"               << "xterm"
          << "crosshair"           << "left_ptr_watch"      << "center_ptr"
          << "sb_h_double_arrow"   << "sb_v_double_arrow"   << "fleur"
          << "top_left_corner"     << "top_side"            << "top_right_corner"
          << "right_side"          << "bottom_right_corner" << "bottom_side"
          << "bottom_left_corner"  << "left_side"           << "question_arrow"
          << "pirate";

    foreach (const QString &name, names) {
        XFixesChangeCursorByName(QX11Info::display(), theme.loadCursor(name, 0), QFile::encodeName(name));
    }

#else
    KMessageBox::information(this,
                                 i18n("You have to restart KDE for cursor changes to take effect."),
                                 i18n("Cursor Settings Changed"), "CursorSettingsChanged");
#endif
}

QDir KCMLookandFeel::cursorThemeDir(const QString &theme, const int depth)
{
    // Prevent infinite recursion
    if (depth > 10) {
        return QDir();
    }

    // Search each icon theme directory for 'theme'
    foreach (const QString &baseDir, cursorSearchPaths()) {
        QDir dir(baseDir);
        if (!dir.exists() || !dir.cd(theme)) {
            continue;
        }

        // If there's a cursors subdir, we'll assume this is a cursor theme
        if (dir.exists("cursors")) {
            return dir;
        }

        // If the theme doesn't have an index.theme file, it can't inherit any themes.
        if (!dir.exists("index.theme")) {
            continue;
        }

        // Open the index.theme file, so we can get the list of inherited themes
        KConfig config(dir.path() + "/index.theme", KConfig::NoGlobals);
        KConfigGroup cg(&config, "Icon Theme");

        // Recurse through the list of inherited themes, to check if one of them
        // is a cursor theme.
        QStringList inherits = cg.readEntry("Inherits", QStringList());
        foreach (const QString &inherit, inherits) {
            // Avoid possible DoS
            if (inherit == theme) {
                continue;
            }

            if (cursorThemeDir(inherit, depth + 1).exists()) {
                return dir;
            }
        }
    }

    return QDir();
}

const QStringList KCMLookandFeel::cursorSearchPaths()
{
    if (!m_cursorSearchPaths.isEmpty())
        return m_cursorSearchPaths;

#if XCURSOR_LIB_MAJOR == 1 && XCURSOR_LIB_MINOR < 1
    // These are the default paths Xcursor will scan for cursor themes
    QString path("~/.icons:/usr/share/icons:/usr/share/pixmaps:/usr/X11R6/lib/X11/icons");

    // If XCURSOR_PATH is set, use that instead of the default path
    char *xcursorPath = std::getenv("XCURSOR_PATH");
    if (xcursorPath)
        path = xcursorPath;
#else
    // Get the search path from Xcursor
    QString path = XcursorLibraryPath();
#endif

    // Separate the paths
    m_cursorSearchPaths = path.split(':', QString::SkipEmptyParts);

    // Remove duplicates
    QMutableStringListIterator i(m_cursorSearchPaths);
    while (i.hasNext())
    {
        const QString path = i.next();
        QMutableStringListIterator j(i);
        while (j.hasNext())
            if (j.next() == path)
                j.remove();
    }

    // Expand all occurrences of ~/ to the home dir
    m_cursorSearchPaths.replaceInStrings(QRegExp("^~\\/"), QDir::home().path() + '/');
    return m_cursorSearchPaths;
}

void KCMLookandFeel::setApplyColors(bool apply)
{
    if (m_applyColors == apply) {
        return;
    }

    m_applyColors = apply;
    emit applyColorsChanged();
}

bool KCMLookandFeel::applyColors() const
{
    return m_applyColors;
}

void KCMLookandFeel::setApplyWidgetStyle(bool apply)
{
    if (m_applyWidgetStyle == apply) {
        return;
    }

    m_applyWidgetStyle = apply;
    emit applyWidgetStyleChanged();
}

bool KCMLookandFeel::applyWidgetStyle() const
{
    return m_applyWidgetStyle;
}

void KCMLookandFeel::setApplyIcons(bool apply)
{
    if (m_applyIcons == apply) {
        return;
    }

    m_applyIcons = apply;
    emit applyIconsChanged();
}

bool KCMLookandFeel::applyIcons() const
{
    return m_applyIcons;
}

void KCMLookandFeel::setApplyPlasmaTheme(bool apply)
{
    if (m_applyPlasmaTheme == apply) {
        return;
    }

    m_applyPlasmaTheme = apply;
    emit applyPlasmaThemeChanged();
}

bool KCMLookandFeel::applyPlasmaTheme() const
{
    return m_applyPlasmaTheme;
}

#include "kcm.moc"
