//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: ExamplePlugin                      #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#                             COPYRIGHT: XXX                             #
//#                                                                        #
//##########################################################################

// First:
//	Replace all occurrences of 'ExamplePlugin' by your own plugin class name in this file.
//	This includes the resource path to info.json in the constructor.

// Second:
//	Open ExamplePlugin.qrc, change the "prefix" and the icon filename for your plugin.
//	Change the name of the file to <yourPluginName>.qrc

// Third:
//	Open the info.json file and fill in the information about the plugin.
//	 "type" should be one of: "Standard", "GL", or "I/O" (required)
//	 "name" is the name of the plugin (required)
//	 "icon" is the Qt resource path to the plugin's icon (from the .qrc file)
//	 "description" is used as a tootip if the plugin has actions and is displayed in the plugin dialog
//	 "authors", "maintainers", and "references" show up in the plugin dialog as well

#include <QtGui>

#include "ExamplePlugin.h"



//by kk
#include <ccPointCloud.h>
#include <QInputDialog>
#include <ccProgressDialog.h>

// Default constructor:
//	- pass the Qt resource path to the info.json file (from <yourPluginName>.qrc file) 
//  - constructor should mainly be used to initialize actions and other members
ExamplePlugin::ExamplePlugin( QObject *parent )
	: QObject( parent )
	, ccStdPluginInterface( ":/CC/plugin/ExamplePlugin/info.json" )
	, m_action( nullptr )
{
}

// This method should enable or disable your plugin actions
// depending on the currently selected entities ('selectedEntities').
void ExamplePlugin::onNewSelection( const ccHObject::Container &selectedEntities )
{
	if ( m_action == nullptr )
	{
		return;
	}
	
	// If you need to check for a specific type of object, you can use the methods
	// in ccHObjectCaster.h or loop and check the objects' classIDs like this:
	//
	//	for ( ccHObject *object : selectedEntities )
	//	{
	//		if ( object->getClassID() == CC_TYPES::VIEWPORT_2D_OBJECT )
	//		{
	//			// ... do something with the viewports
	//		}
	//	}
	
	// For example - only enable our action if something is selected.
	m_action->setEnabled( !selectedEntities.empty() );
}

// This method returns all the 'actions' your plugin can perform.
// getActions() will be called only once, when plugin is loaded.
QList<QAction *> ExamplePlugin::getActions()
{
	// default action (if it has not been already created, this is the moment to do it)
	if ( !m_action )
	{
		// Here we use the default plugin name, description, and icon,
		// but each action should have its own.
		m_action = new QAction( getName(), this );
		m_action->setToolTip( getDescription() );
		m_action->setIcon( getIcon() );
		
		// Connect appropriate signal
		

		connect( m_action, &QAction::triggered, this, &ExamplePlugin::doAction );
	}

	return { m_action };
}

// This is an example of an action's method called when the corresponding action
// is triggered (i.e. the corresponding icon or menu entry is clicked in CC's
// main interface). You can access most of CC's components (database,
// 3D views, console, etc.) via the 'm_app' variable (see the ccMainAppInterface
// class in ccMainAppInterface.h).
void ExamplePlugin::doAction()
{	
	assert(m_app);
	if (!m_app)
		return;

	//得到点云
	const ccHObject::Container& selectedEntities = m_app->getSelectedEntities();
	size_t selNum = selectedEntities.size();


	//传入参数
	bool isOK;
	QString s_percentage = QInputDialog::getText(NULL, QString::fromLocal8Bit("参数"),
		QString::fromLocal8Bit("输入留下点的百分比%"),
		QLineEdit::Normal,
		"",
		&isOK);
	if (!isOK)
	{
		ccLog::Print("error");
		return;
	}

	//留下点的个数
	double percentage = s_percentage.toDouble();
	if (percentage < 0 || percentage>1)
	{
		ccLog::Print("error percentage allow 0-1");
		return;
	}	

	//加个进度条
	ccProgressDialog pDlg(true, 0);
	CCCoreLib::GenericProgressCallback* progressCb = &pDlg;

	if (progressCb)
	{
		if (progressCb->textCanBeEdited())
		{
			progressCb->setMethodTitle("compute");
			progressCb->setInfo(qPrintable(QString("waiting...")));
		}
		progressCb->update(0);
		progressCb->start();
}
	std::vector<ccHObject*> allCloud;

	//随机数
	qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));

	for (size_t i = 0; i < selNum; ++i)
	{
		ccHObject* ent = selectedEntities[i];

		if (!ent->isA(CC_TYPES::POINT_CLOUD))
		{
			continue;
		}

		ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(selectedEntities[i]);
		//计算点云中心
		CCVector3 weight;

		CCVector3 bmin, bmax;
		cloud->getBoundingBox(bmin, bmax);
		weight = (bmin + bmax) / 2;

		//遍历索引
		std::vector<size_t> m_index(cloud->size());
		//以递增的方式填充
		std::iota(m_index.begin(), m_index.end(), 0);

		//随机一个方向
		double theta = M_PI * (double)qrand() / (double)(RAND_MAX + 1);  //0-pi
		double phi = 2 * M_PI * (double)qrand() / (double)(RAND_MAX + 1);     //0-2pi

		CCVector3 dir(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));

		std::vector<double> m_value;

		for (size_t k = 0; k < m_index.size(); k++)
		{
			CCVector3 P;
			cloud->getPoint(m_index[k], P);
			double temp = (P - weight).dot(dir);
			m_value.push_back(temp);
		}
		//对m_index进行排序
		std::sort(m_index.begin(), m_index.end(), [&m_value](auto i1, auto i2) {return m_value[i1] < m_value[i2]; });
		m_index.erase(m_index.begin(), m_index.begin() + (1 - percentage)*cloud->size());

		//新点云
		ccPointCloud* pcc = new ccPointCloud(QString("cq") + cloud->getName());
		pcc->reserve(m_index.size());
		//pcc->reserveTheNormsTable();

		for (size_t j = 0; j < m_index.size(); j++)
		{
			pcc->addPoint(*cloud->getPoint(m_index[j]));
			//pcc->addNorm(cloud->getPointNormal(m_index[j]));
		}

		allCloud.push_back(pcc);

		progressCb->update((float)100.0*i / selNum);
	}
	if (progressCb)
		progressCb->stop();

	//新建一个文件夹来放点云
	ccHObject* CloudGroup = new ccHObject(QString("CloudGroup"));

	for (size_t i = 0; i < allCloud.size(); i++)
	{
		CloudGroup->addChild(allCloud[i]);
	}


	m_app->addToDB(CloudGroup);
	//刷新
	m_app->refreshAll();
	m_app->updateUI();
}
